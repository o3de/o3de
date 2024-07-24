/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AssetBrowserFavoritesView.h"

#include <AzToolsFramework/AssetBrowser/Favorites/AssetBrowserFavoritesModel.h>
#include <AzToolsFramework/AssetBrowser/Favorites/FavoritesEntryDelegate.h>
#include <AzToolsFramework/AssetBrowser/Favorites/SearchAssetBrowserFavoriteItem.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>

#include <QHeaderView>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        constexpr int MinimumHeight = 25;

        AssetBrowserFavoritesView::AssetBrowserFavoritesView(QWidget* parent)
            : QTreeViewWithStateSaving(parent)
            , m_favoritesModel(new AzToolsFramework::AssetBrowser::AssetBrowserFavoritesModel(parent))
            , m_delegate(new FavoritesEntryDelegate(this))
        {
            setModel(m_favoritesModel.data());
            setItemDelegate(m_delegate.data());
            setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
            setContextMenuPolicy(Qt::CustomContextMenu);
            setMinimumHeight(MinimumHeight);
            setDragEnabled(true);
            setAcceptDrops(true);
            setDragDropMode(QAbstractItemView::DragDrop);
            setDropIndicatorShown(true);
            setContextMenuPolicy(Qt::CustomContextMenu);

            m_favoritesModel->SetParentView(this);

            m_delegate.data()->SetShowFavoriteIcons(true);

            header()->hide();

            connect(m_delegate.data(), &EntryDelegate::RenameEntry, this, &AssetBrowserFavoritesView::AfterRename);
            connect(this, &QTreeView::customContextMenuRequested, this, &AssetBrowserFavoritesView::OnContextMenu);
            connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &AssetBrowserFavoritesView::SelectionChanged);

            connect(this, &QTreeView::expanded, this, &AssetBrowserFavoritesView::expanded);
            connect(this, &QTreeView::collapsed, this, &AssetBrowserFavoritesView::collapsed);
            connect(this, &QAbstractItemView::clicked, this, &AssetBrowserFavoritesView::ItemClicked);

            m_currentHeight = this->height();
        }

        void AssetBrowserFavoritesView::SetSearchWidget(SearchWidget* searchWidget)
        {
            m_favoritesModel.data()->SetSearchWidget(searchWidget);
        }

        void AssetBrowserFavoritesView::OnContextMenu([[maybe_unused]] const QPoint& point)
        {
            auto selectedAssets = selectedIndexes();

            if (selectedAssets.isEmpty())
            {
                return;
            }

            QModelIndex selected = selectedAssets.at(0);

            if (!selected.parent().isValid())
            {
                // This is the "favorites" folder.
                return;
            }

            AssetBrowserFavoriteItem* favoriteItem = static_cast<AssetBrowserFavoriteItem*>(selected.internalPointer());

            AssetBrowserFavoriteItem::FavoriteType favoriteType = favoriteItem->GetFavoriteType();
            QMenu menu(this);

            if (favoriteType == AssetBrowserFavoriteItem::FavoriteType::AssetBrowserEntry)
            {
                AZStd::vector<const AssetBrowserEntry*> selectedEntries;

                EntryAssetBrowserFavoriteItem* favoriteEntry = static_cast<EntryAssetBrowserFavoriteItem*>(favoriteItem);
                selectedEntries.push_back(favoriteEntry->GetEntry());

                AssetBrowserInteractionNotificationBus::Broadcast(
                    &AssetBrowserInteractionNotificationBus::Events::AddContextMenuActions, this, &menu, selectedEntries);
            }
            else
            {
                if (!m_favoritesModel.data()->GetIsSearchDisabled())
                {
                    menu.addAction(
                        QObject::tr("Apply to Asset Browser"),
                        [this, favoriteItem]()
                        {
                            m_favoritesModel.data()->Select(favoriteItem);
                        });
                    menu.addAction(
                        QObject::tr("Update Saved Search"),
                        [this, favoriteItem]()
                        {
                            SearchAssetBrowserFavoriteItem* searchFavorite = static_cast<SearchAssetBrowserFavoriteItem*>(favoriteItem);
                            m_favoritesModel.data()->UpdateSearchItem(searchFavorite);
                        });
                }
                menu.addAction(
                    QObject::tr("Remove from Favorites"),
                    [favoriteItem]()
                    {
                        AssetBrowserFavoriteRequestBus::Broadcast(&AssetBrowserFavoriteRequestBus::Events::RemoveFromFavorites, favoriteItem);
                    });

                menu.addAction(
                    QObject::tr("Rename"),
                    [this, selected]()
                    {
                        edit(selected);
                    });
            }

            menu.exec(QCursor::pos());
        }

        void AssetBrowserFavoritesView::AfterRename(QString newName)
        {
            auto selectedAssets = selectedIndexes();

            if (selectedAssets.isEmpty())
            {
                return;
            }

            QModelIndex selected = selectedAssets.at(0);
            AssetBrowserFavoriteItem* favoriteItem = static_cast<AssetBrowserFavoriteItem*>(selected.internalPointer());

            AssetBrowserFavoriteItem::FavoriteType favoriteType = favoriteItem->GetFavoriteType();

            if (favoriteType == AssetBrowserFavoriteItem::FavoriteType::Search)
            {
                SearchAssetBrowserFavoriteItem* searchItem = static_cast<SearchAssetBrowserFavoriteItem*>(favoriteItem);
                searchItem->SetName(newName);

                AssetBrowserFavoriteRequestBus::Broadcast(&AssetBrowserFavoriteRequestBus::Events::SaveFavorites);

            }
        }

        void AssetBrowserFavoritesView::dragMoveEvent(QDragMoveEvent* event)
        {
            if (event->mimeData()->hasFormat(AssetBrowserEntry::GetMimeType()) ||
                event->mimeData()->hasFormat(FolderAssetBrowserEntry::GetMimeType()))
            {
                event->accept();
                return;
            }
            event->ignore();
        }

        void AssetBrowserFavoritesView::mouseDoubleClickEvent(QMouseEvent* event)
        {
            const auto p = event->pos();
            if (auto idx = indexAt(p); idx.isValid())
            {
                selectionModel()->select(idx, QItemSelectionModel::SelectionFlag::ClearAndSelect | QItemSelectionModel::Rows);

                AssetBrowserFavoriteItem* favoriteItem = static_cast<AssetBrowserFavoriteItem*>(idx.internalPointer());

                m_favoritesModel.data()->Select(favoriteItem);
            }
        }

        int slideroffset = 5;

        void AssetBrowserFavoritesView::resizeEvent(QResizeEvent* event)
        {
            if (isExpanded(m_favoritesModel->GetTopLevelIndex()))
            {
                m_currentHeight = event->size().height();
            }

            QTreeViewWithStateSaving::resizeEvent(event);
        }

        void AssetBrowserFavoritesView::collapsed(const QModelIndex& index)
        {
            if (!index.parent().isValid())
            {
                emit(setFavoritesWindowHeight(MinimumHeight));
            }
        }

        int hosft = 10;

        void AssetBrowserFavoritesView::expanded(const QModelIndex& index)
        {
            if (!index.parent().isValid())
            {
                if (height() == MinimumHeight)
                {
                    emit(setFavoritesWindowHeight(m_currentHeight + hosft));
                }
            }
        }

        void AssetBrowserFavoritesView::drawBranches(QPainter* painter, const QRect& rect, const QModelIndex& index) const
        {
            if (!index.parent().isValid() && !selectedIndexes().contains(index))
            {
                painter->fillRect(rect, 0x333333);
            }

            QTreeView::drawBranches(painter, rect, index);
        }

        void AssetBrowserFavoritesView::SetSearchDisabled(bool disabled)
        {
            if (disabled)
            {
                m_favoritesModel->DisableSearchItems();
            }
            else
            {
                m_favoritesModel->EnableSearchItems();
            }
        }

        
        void AssetBrowserFavoritesView::SelectionChanged(const QItemSelection& selected, [[maybe_unused]] const QItemSelection& deselected)
        {
            NotifySelection(selected);
        }

        void AssetBrowserFavoritesView::NotifySelection(const QItemSelection& selected)
        {
            // if we select 1 thing, give the previewer a chance to view it.  Favorites is a cell-by-cell selection
            // so it won't select an entire row, ie, every selection will be its own row so its not necessary to count rows, as that will
            // be the same as the count of the selected indexes.
            QModelIndexList selectedIndexes = selected.indexes();
            if (selectedIndexes.size() == 1)
            {
                const QModelIndex index = selectedIndexes[0];
                if (index.isValid())
                {
                    auto* favorite = index.data(AssetBrowserModel::Roles::EntryRole).value<const AssetBrowserFavoriteItem*>();
                    if ((favorite)&&(favorite->GetFavoriteType() == AssetBrowserFavoriteItem::FavoriteType::AssetBrowserEntry))
                    {
                        const EntryAssetBrowserFavoriteItem* entryItem = static_cast<const EntryAssetBrowserFavoriteItem*>(favorite);
                        AssetBrowserPreviewRequestBus::Broadcast(&AssetBrowserPreviewRequest::PreviewAsset, entryItem->GetEntry());
                        return;
                    }
                }
            }

            // note that if we don't early return above, we must clear the preview, as we have selected multiple items, or 0 items.
            AssetBrowserPreviewRequestBus::Broadcast(&AssetBrowserPreviewRequest::ClearPreview);
        }

        void AssetBrowserFavoritesView::ItemClicked(const QModelIndex& index)
        {
            // if we click on an item that was already selected, notify anyway, as we want the behavior to be that
            // it refreshes the gui when you do that.
            if (selectionModel()->isSelected(index))
            {
                NotifySelection(selectionModel()->selection());
            }
        }

        AssetBrowserFavoritesModel* AssetBrowserFavoritesView::GetModel()
        {
            return m_favoritesModel.data();
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework

#include "AssetBrowser/Favorites/moc_AssetBrowserFavoritesView.cpp"
