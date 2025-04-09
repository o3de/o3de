/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserThumbnailView.h>

#include <AzToolsFramework/ActionManager/HotKey/HotKeyManagerInterface.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserThumbnailViewProxyModel.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntryUtils.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorContextIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerUtils.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserTreeView.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserViewUtils.h>
#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/ProductAssetBrowserEntry.h>

#include <AzCore/Interface/Interface.h>

#include <AzQtComponents/Components/Widgets/AssetFolderThumbnailView.h>
#include <AzQtComponents/DragAndDrop/MainWindowDragAndDrop.h>

#if !defined(Q_MOC_RUN)
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QVBoxLayout>
#endif

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        static constexpr const char* const ThumbnailViewMainViewName = "AssetBrowserThumbnailView_main";

        AssetBrowserThumbnailView::AssetBrowserThumbnailView(QWidget* parent)
            : QWidget(parent)
            , m_thumbnailViewWidget(new AzQtComponents::AssetFolderThumbnailView(parent))
            , m_thumbnailViewProxyModel(new AssetBrowserThumbnailViewProxyModel(parent))
            , m_assetFilterModel(new AssetBrowserFilterModel(parent))
        {
            // Using our own instance of AssetBrowserFilterModel to be able to show also files when the main model
            // only lists directories, and at the same time get sort and filter entries features from AssetBrowserFilterModel.
            m_thumbnailViewProxyModel->setSourceModel(m_assetFilterModel);
            m_thumbnailViewWidget->setModel(m_thumbnailViewProxyModel);
            SetSortMode(AssetBrowserEntry::AssetEntrySortMode::Name);

            connect(
                m_thumbnailViewWidget,
                &AzQtComponents::AssetFolderThumbnailView::selectionChangedSignal,
                this,
                [this](
                    const QItemSelection& selected,
                    const QItemSelection& deselected)
                {
                    Q_EMIT selectionChangedSignal(selected, deselected);
                });

            connect(
                m_thumbnailViewWidget,
                &AzQtComponents::AssetFolderThumbnailView::clicked,
                this,
                [this](const QModelIndex& index)
                {
                    auto indexData = index.data(AssetBrowserModel::Roles::EntryRole).value<const AssetBrowserEntry*>();
                    emit entryClicked(indexData);
                    // if we click on an index that is already selected, refresh the selection so that other views update.
                    if (m_thumbnailViewWidget->selectionModel()->isSelected(index))
                    {
                        // user clicked on the same entry as before, so make sure that the associated item is previewed
                        // in case they clicked on something else on the GUI and it was lost.
                        Q_EMIT selectionChangedSignal(m_thumbnailViewWidget->selectionModel()->selection(), {});
                    }
                });

            connect(
                m_thumbnailViewWidget,
                &AzQtComponents::AssetFolderThumbnailView::deselected, this, []
                {
                    AssetBrowserPreviewRequestBus::Broadcast(&AssetBrowserPreviewRequest::ClearPreview);
                });

            connect(
                m_thumbnailViewWidget,
                &AzQtComponents::AssetFolderThumbnailView::doubleClicked,
                this,
                [this](const QModelIndex& index)
                {
                    auto indexData = index.data(AssetBrowserModel::Roles::EntryRole).value<const AssetBrowserEntry*>();
                    emit entryDoubleClicked(indexData);
                });

            connect(
                m_thumbnailViewWidget,
                &AzQtComponents::AssetFolderThumbnailView::contextMenu,
                this,
                [this]()
                {
                    auto entries = AZStd::move(GetSelectedAssets());
                    if (entries.empty() && m_assetTreeView)
                    {
                        // Tree has the current open folder selected if context is valid (not searching, etc)
                        const auto treeSelection = m_assetTreeView->selectionModel()->selectedIndexes();
                        if (!treeSelection.empty())
                        {
                            m_thumbnailViewWidget->selectionModel()->select(
                                treeSelection.first(), QItemSelectionModel::SelectionFlag::ClearAndSelect);
                            entries = AZStd::move(GetSelectedAssets());
                        }
                    }

                    QMenu menu(this);

                    if (entries.size() == 1)
                    {
                        const AssetBrowserEntry* entry = entries.at(0);

                        if (m_thumbnailViewWidget->InSearchResultsMode())
                        {
                            auto action = menu.addAction(tr("Show In Folder"));
                            connect(
                                action,
                                &QAction::triggered,
                                this,
                                [this, entry]()
                                {
                                    emit showInFolderTriggered(entry);
                                });
                            menu.addSeparator();
                        }
                    }

                    AssetBrowserInteractionNotificationBus::Broadcast(
                        &AssetBrowserInteractionNotificationBus::Events::AddContextMenuActions, this, &menu, entries);

                    if (!menu.isEmpty())
                    {
                        menu.exec(QCursor::pos());
                    }
                });

            connect(m_thumbnailViewWidget, &AzQtComponents::AssetFolderThumbnailView::afterRename, this, &AssetBrowserThumbnailView::AfterRename);


              QAction* deleteAction = new QAction("Delete Action", this);
              deleteAction->setShortcut(QKeySequence::Delete);
              deleteAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
              connect(
                  deleteAction,
                  &QAction::triggered,
                  this,
                  [this]()
                  {
                      DeleteEntries();
                  });
              addAction(deleteAction);

              QAction* renameAction = new QAction("Rename Action", this);
              renameAction->setShortcut(Qt::Key_F2);
              renameAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
              connect(
                  renameAction,
                  &QAction::triggered,
                  this,
                  [this]()
                  {
                      RenameEntry();
                  });
              addAction(renameAction);

              QAction* duplicateAction = new QAction("Duplicate Action", this);
              duplicateAction->setShortcut(QKeySequence("Ctrl+D"));
              duplicateAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
              connect(
                  duplicateAction,
                  &QAction::triggered,
                  this,
                  [this]()
                  {
                      DuplicateEntries();
                  });
              addAction(duplicateAction);

            // Track the root index on the proxy model as well so it can provide info such as whether an entry is first level or not
            connect(
                m_thumbnailViewWidget,
                &AzQtComponents::AssetFolderThumbnailView::rootIndexChanged,
                m_thumbnailViewProxyModel,
                &AssetBrowserThumbnailViewProxyModel::SetRootIndex);

            auto layout = new QVBoxLayout();
            layout->setContentsMargins(0, 0, 0, 0);
            layout->addWidget(m_thumbnailViewWidget);
            setLayout(layout);

            setAcceptDrops(true);

            AssignWidgetToActionContextHelper(EditorIdentifiers::EditorAssetBrowserActionContextIdentifier, this);
        }

        AssetBrowserThumbnailView::~AssetBrowserThumbnailView()
        {
            RemoveWidgetFromActionContextHelper(EditorIdentifiers::EditorAssetBrowserActionContextIdentifier, this);
        }

        AzQtComponents::AssetFolderThumbnailView* AssetBrowserThumbnailView::GetThumbnailViewWidget() const
        {
            return m_thumbnailViewWidget;
        }

        void AssetBrowserThumbnailView::SetName(const QString& name)
        {
            m_name = name;
        }

        QString& AssetBrowserThumbnailView::GetName()
        {
            return m_name;
        }

        void AssetBrowserThumbnailView::SetIsAssetBrowserMainView()
        {
            SetName(ThumbnailViewMainViewName);
        }

        bool AssetBrowserThumbnailView::GetIsAssetBrowserMainView()
        {
            return GetName() == ThumbnailViewMainViewName;
        }

        void AssetBrowserThumbnailView::SetThumbnailActiveView(bool isActiveView)
        {
            m_isActiveView = isActiveView;
        }

        bool AssetBrowserThumbnailView::GetThumbnailActiveView()
        {
            return m_isActiveView;
        }

        void AssetBrowserThumbnailView::DeleteEntries()
        {
            const auto& entries = GetSelectedAssets(false); // you cannot delete product files.
            AssetBrowserViewUtils::DeleteEntries(entries, this);
        }

        void AssetBrowserThumbnailView::MoveEntries()
        {
            const auto& entries = GetSelectedAssets(false); // you cannot move product files.
            AssetBrowserViewUtils::MoveEntries(entries, this);
        }

        void AssetBrowserThumbnailView::DuplicateEntries()
        {
            const auto& entries = GetSelectedAssets(false); // you may not duplicate product files.
            AssetBrowserViewUtils::DuplicateEntries(entries);
        }

        void AssetBrowserThumbnailView::RenameEntry()
        {
            const auto& entries = GetSelectedAssets(false); // you cannot rename product files.
            if (AssetBrowserViewUtils::RenameEntry(entries, this))
            {
                QModelIndex selectedIndex = m_thumbnailViewWidget->selectionModel()->selectedIndexes()[0];
                m_thumbnailViewWidget->edit(selectedIndex);
            }
        }

        void AssetBrowserThumbnailView::AfterRename(QString newVal)
        {
            const auto& entries = GetSelectedAssets(false); // you cannot rename product files.
            AssetBrowserViewUtils::AfterRename(newVal, entries, this);
        }

        AZStd::vector<const AssetBrowserEntry*> AssetBrowserThumbnailView::GetSelectedAssets(bool includeProducts) const
        {
            AZStd::vector<const AssetBrowserEntry*> entries;
            if (m_thumbnailViewWidget->selectionModel())
            {
                AssetBrowserModel::SourceIndexesToAssetDatabaseEntries(m_thumbnailViewWidget->selectionModel()->selectedIndexes(), entries);
                if (!includeProducts)
                {
                    AZStd::erase_if(
                        entries,
                        [&](const AssetBrowserEntry* entry) -> bool
                        {
                            return entry->GetEntryType() == AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType::Product;
                        });
                }
            }
            return entries;
        }

        void AssetBrowserThumbnailView::SetAssetTreeView(AssetBrowserTreeView* treeView)
        {
            if (m_assetTreeView)
            {
                disconnect(m_assetTreeView, &AssetBrowserTreeView::selectionChangedSignal, this, nullptr);
                auto treeViewFilterModel = qobject_cast<AssetBrowserFilterModel*>(m_assetTreeView->model());
                if (treeViewFilterModel)
                {
                    disconnect(
                        treeViewFilterModel,
                        &AssetBrowserFilterModel::filterChanged,
                        this,
                        &AssetBrowserThumbnailView::UpdateFilterInLocalFilterModel);
                }
            }

            m_assetTreeView = treeView;
            if (!m_assetTreeView)
            {
                return;
            }

            m_assetTreeView->SetAttachedThumbnailView(this);

            auto treeViewFilterModel = qobject_cast<AssetBrowserFilterModel*>(m_assetTreeView->model());
            if (!treeViewFilterModel)
            {
                return;
            }

            auto treeViewModel = qobject_cast<AssetBrowserModel*>(treeViewFilterModel->sourceModel());
            if (!treeViewModel)
            {
                return;
            }

            m_assetFilterModel->setSourceModel(treeViewModel);
            UpdateFilterInLocalFilterModel();

            connect(
                treeViewFilterModel,
                &AssetBrowserFilterModel::filterChanged,
                this,
                &AssetBrowserThumbnailView::UpdateFilterInLocalFilterModel);

            connect(
                m_assetTreeView,
                &AssetBrowserTreeView::selectionChangedSignal,
                this,
                &AssetBrowserThumbnailView::HandleTreeViewSelectionChanged);
        }

        void AssetBrowserThumbnailView::setSelectionMode(QAbstractItemView::SelectionMode mode)
        {
            m_thumbnailViewWidget->setSelectionMode(mode);
        }

        QAbstractItemView::SelectionMode AssetBrowserThumbnailView::selectionMode() const
        {
            return m_thumbnailViewWidget->selectionMode();
        }

        void AssetBrowserThumbnailView::dragEnterEvent(QDragEnterEvent* event)
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

        void AssetBrowserThumbnailView::dragMoveEvent(QDragMoveEvent* event)
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

        void AssetBrowserThumbnailView::dropEvent(QDropEvent* event)
        {
            if (event->mimeData()->hasFormat(SourceAssetBrowserEntry::GetMimeType()) ||
                event->mimeData()->hasFormat(ProductAssetBrowserEntry::GetMimeType()))
            {
                event->accept();
                return;
            }

            const AssetBrowserEntry* item = m_thumbnailViewProxyModel->mapToSource(m_thumbnailViewProxyModel->GetRootIndex())
                                                .data(AssetBrowserModel::Roles::EntryRole)
                                                .value<const AssetBrowserEntry*>();
            AZStd::string pathName = item->GetFullPath();
            

            using namespace AzQtComponents;
            DragAndDropContextBase context;
            DragAndDropEventsBus::Event(DragAndDropContexts::EditorMainWindow, &DragAndDropEvents::DropAtLocation, event, context, QString(pathName.data()));
        }

        void AssetBrowserThumbnailView::dragLeaveEvent(QDragLeaveEvent* event)
        {
            using namespace AzQtComponents;
            DragAndDropEventsBus::Event(DragAndDropContexts::EditorMainWindow, &DragAndDropEvents::DragLeave, event);
        }

        void AssetBrowserThumbnailView::HandleTreeViewSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
        {
            Q_UNUSED(deselected);

            auto treeViewFilterModel = qobject_cast<AssetBrowserFilterModel*>(m_assetTreeView->model());
            if (!treeViewFilterModel)
            {
                return;
            }

            const auto& selectedIndexes = selected.indexes();
            if (!selectedIndexes.empty())
            {
                const auto newRootIndex = m_thumbnailViewProxyModel->mapFromSource(
                    m_assetFilterModel->mapFromSource(treeViewFilterModel->mapToSource(selectedIndexes[0])));
                m_thumbnailViewWidget->setRootIndex(newRootIndex);
            }
            else
            {
                m_thumbnailViewWidget->setRootIndex({});
            }

            m_assetFilterModel->sort(0, Qt::DescendingOrder);
            m_assetFilterModel->setDynamicSortFilter(true);
        }

        void AssetBrowserThumbnailView::OpenItemForEditing(const QModelIndex& index)
        {
            QModelIndex proxyIndex = m_thumbnailViewProxyModel->mapFromSource(
                m_assetFilterModel->mapFromSource(index));

            if (proxyIndex.isValid())
            {
                m_thumbnailViewWidget->selectionModel()->select(proxyIndex, QItemSelectionModel::SelectionFlag::ClearAndSelect);

                m_thumbnailViewWidget->scrollTo(proxyIndex, QAbstractItemView::ScrollHint::PositionAtCenter);

                RenameEntry();
            }
        }

        void AssetBrowserThumbnailView::UpdateFilterInLocalFilterModel()
        {
            if (!m_assetTreeView)
            {
                return;
            }

            auto treeViewFilterModel = qobject_cast<AssetBrowserFilterModel*>(m_assetTreeView->model());
            if (!treeViewFilterModel)
            {
                return;
            }

            auto filter = qobject_cast<const CompositeFilter*>(treeViewFilterModel->GetFilter().get());
            if (!filter)
            {
                return;
            }

            bool hasString{ false };
            const QString tagString("String");
            const QString tagFolder("Folder");
            auto clonedFilter = new CompositeFilter(CompositeFilter::LogicOperatorType::AND);
            for (const auto& subFilter : filter->GetSubFilters())
            {
                if (subFilter->GetTag() == tagString)
                {
                    auto stringCompFilter = qobject_cast<const CompositeFilter*>(subFilter.get());
                    hasString |= stringCompFilter && !stringCompFilter->GetSubFilters().empty();
                }

                // Skip the folder filter on the thumbnail view so that we can see files
                if (subFilter->GetTag() != tagFolder)
                {
                    clonedFilter->AddFilter(FilterConstType(subFilter->Clone()));
                }
            }

            // Switch between "search mode" where all results in the asset folder tree are shown,
            // and "normal mode", where only contents for a single folder are shown, depending on
            // whether there is an active string search ongoing.
            m_thumbnailViewProxyModel->SetShowSearchResultsMode(hasString);
            m_thumbnailViewWidget->SetShowSearchResultsMode(hasString);

            if (hasString)
            {
                // With string searches enabled we disable filter propagation to only show exact mattress.
                for (auto subFilter : clonedFilter->GetSubFilters())
                {
                    if (auto clonedCompositeFilter = qobject_cast<const CompositeFilter*>(subFilter.get()); clonedCompositeFilter)
                    {
                        auto modifiedCompositeFilter = const_cast<CompositeFilter*>(clonedCompositeFilter);
                        modifiedCompositeFilter->SetFilterPropagation(AssetBrowserEntryFilter::PropagateDirection::None);
                    }
                }

                // With string searches enabled we only want to show sources and products in this view.
                auto customTypeFilter = new CustomFilter(
                    [](const AssetBrowserEntry* entry)
                    {
                        switch (entry->GetEntryType())
                        {
                        case AssetBrowserEntry::AssetEntryType::Product:
                        case AssetBrowserEntry::AssetEntryType::Source:
                            return true;
                        }
                        return false;
                    });
                clonedFilter->AddFilter(FilterConstType(customTypeFilter));
            }

            clonedFilter->SetFilterPropagation(AssetBrowserEntryFilter::PropagateDirection::Down);
            m_assetFilterModel->SetFilter(FilterConstType(clonedFilter));
        }

        void AssetBrowserThumbnailView::SetSortMode(const AssetBrowserEntry::AssetEntrySortMode mode)
        {
            m_assetFilterModel->SetSortMode(mode);
            m_assetFilterModel->sort(0, Qt::DescendingOrder);
            m_assetFilterModel->setDynamicSortFilter(true);
        }

       AssetBrowserEntry::AssetEntrySortMode AssetBrowserThumbnailView::GetSortMode() const
        {
            return m_assetFilterModel->GetSortMode();
        }
        
        void AssetBrowserThumbnailView::SelectEntry(QString assetName)
        {
            QModelIndex rootIndex = m_thumbnailViewProxyModel->GetRootIndex();

            auto model = GetThumbnailViewWidget()->model();

            for (int rowIndex = 0; rowIndex < model->rowCount(rootIndex); rowIndex++)
            {
                auto index = model->index(rowIndex, 0, rootIndex);
                if (!index.isValid())
                {
                    continue;
                }

                auto str = index.data().toString();
                if (assetName == str)
                {
                    m_thumbnailViewWidget->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
                    m_thumbnailViewWidget->scrollTo(index, QAbstractItemView::ScrollHint::PositionAtCenter);
                }
            }
        }

        void AssetBrowserThumbnailView::SetSearchString(const QString& searchString)
        {
            m_thumbnailViewProxyModel->SetSearchString(searchString);
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework
