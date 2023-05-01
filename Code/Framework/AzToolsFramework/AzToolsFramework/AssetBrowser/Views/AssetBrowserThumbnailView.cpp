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
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorContextIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerUtils.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserTreeView.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserViewUtils.h>

#include <AzCore/Interface/Interface.h>

#include <AzQtComponents/Components/Widgets/AssetFolderThumbnailView.h>

#if !defined(Q_MOC_RUN)
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
            m_assetFilterModel->sort(0, Qt::DescendingOrder);
            m_thumbnailViewProxyModel->setSourceModel(m_assetFilterModel);
            m_thumbnailViewWidget->setModel(m_thumbnailViewProxyModel);

            connect(
                m_thumbnailViewWidget,
                &AzQtComponents::AssetFolderThumbnailView::clicked,
                this,
                [this](const QModelIndex& index)
                {
                    auto indexData = index.data(AssetBrowserModel::Roles::EntryRole).value<const AssetBrowserEntry*>();
                    AssetBrowserPreviewRequestBus::Broadcast(&AssetBrowserPreviewRequest::PreviewAsset, indexData);
                    emit entryClicked(indexData);
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
                [this](const QModelIndex& index)
                {
                    if (index.isValid())
                    {
                        QMenu menu(this);
                        const AssetBrowserEntry* entry = index.data(AssetBrowserModel::Roles::EntryRole).value<const AssetBrowserEntry*>();
                        AZStd::vector<const AssetBrowserEntry*> entries{ entry };
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
                        AssetBrowserInteractionNotificationBus::Broadcast(
                            &AssetBrowserInteractionNotificationBus::Events::AddContextMenuActions, this, &menu, entries);

                        if (!menu.isEmpty())
                        {
                            menu.exec(QCursor::pos());
                        }
                    }
                    else if (!index.isValid() && m_assetTreeView)
                    {
                        m_assetTreeView->OnContextMenu(QCursor::pos());
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
            layout->addWidget(m_thumbnailViewWidget);
            setLayout(layout);

            if (AzToolsFramework::IsNewActionManagerEnabled())
            {
                AssignWidgetToActionContextHelper(EditorIdentifiers::EditorAssetBrowserActionContextIdentifier, this);
            }
        }

        AssetBrowserThumbnailView::~AssetBrowserThumbnailView()
        {
            if (AzToolsFramework::IsNewActionManagerEnabled())
            {
                RemoveWidgetFromActionContextHelper(EditorIdentifiers::EditorAssetBrowserActionContextIdentifier, this);
            }
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
            auto entries = GetSelectedAssets(false); // you cannot delete product files.

            AssetBrowserViewUtils::DeleteEntries(entries, this);
        }

        void AssetBrowserThumbnailView::MoveEntries()
        {
            auto entries = GetSelectedAssets(false); // you cannot move product files.

            AssetBrowserViewUtils::MoveEntries(entries, this);
        }

        void AssetBrowserThumbnailView::DuplicateEntries()
        {
            auto entries = GetSelectedAssets(false); // you may not duplicate product files.
            AssetBrowserViewUtils::DuplicateEntries(entries);
        }

        void AssetBrowserThumbnailView::RenameEntry()
        {
            auto entries = GetSelectedAssets(false); // you cannot rename product files.

            if (AssetBrowserViewUtils::RenameEntry(entries, this))
            {
                QModelIndex selectedIndex = m_thumbnailViewWidget->selectionModel()->selectedIndexes()[0];
                m_thumbnailViewWidget->edit(selectedIndex);
            }
        }

        void AssetBrowserThumbnailView::AfterRename(QString newVal)
        {
            auto entries = GetSelectedAssets(false); // you cannot rename product files.

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
                    entries.erase(
                        AZStd::remove_if(
                            entries.begin(),
                            entries.end(),
                            [&](const AssetBrowserEntry* entry) -> bool
                            {
                                return entry->GetEntryType() ==
                                    AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType::Product;
                            }),
                        entries.end());
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

        void AssetBrowserThumbnailView::HandleTreeViewSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
        {
            Q_UNUSED(deselected);

            auto treeViewFilterModel = qobject_cast<AssetBrowserFilterModel*>(m_assetTreeView->model());
            if (!treeViewFilterModel)
            {
                return;
            }

            auto selectedIndexes = selected.indexes();
            if (selectedIndexes.count() > 0)
            {
                auto newRootIndex = m_thumbnailViewProxyModel->mapFromSource(
                    m_assetFilterModel->mapFromSource(treeViewFilterModel->mapToSource(selectedIndexes[0])));
                m_thumbnailViewWidget->setRootIndex(newRootIndex);
            }
            else
            {
                m_thumbnailViewWidget->setRootIndex({});
            }
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

            auto filterCopy = new CompositeFilter(CompositeFilter::LogicOperatorType::AND);
            for (const auto& subFilter : filter->GetSubFilters())
            {
                // Switch between "search mode" where all results in the asset folder tree are shown,
                // and "normal mode", where only contents for a single folder are shown, depending on
                // whether there is an active string search ongoing.
                if (subFilter->GetTag() == "String")
                {
                    auto stringCompFilter = qobject_cast<const CompositeFilter*>(subFilter.get());
                    if (!stringCompFilter)
                    {
                        continue;
                    }

                    auto stringSubFilters = stringCompFilter->GetSubFilters();

                    m_thumbnailViewProxyModel->SetShowSearchResultsMode(stringSubFilters.count() != 0);
                    m_thumbnailViewWidget->SetShowSearchResultsMode(stringSubFilters.count() != 0);
                }

                // Skip the folder filter on the thumbnail view so that we can see files
                if (subFilter->GetTag() != "Folder")
                {
                    filterCopy->AddFilter(subFilter);
                }
            }
            filterCopy->SetFilterPropagation(AssetBrowserEntryFilter::Down);

            m_assetFilterModel->SetFilter(FilterConstType(filterCopy));
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework
