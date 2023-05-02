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

#include <QHeaderView>
#include <QMenu>
#include <QMouseEvent>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        AssetBrowserFavoritesView::AssetBrowserFavoritesView(QWidget* parent)
            : QTreeViewWithStateSaving(parent)
            , m_favoritesModel(new AzToolsFramework::AssetBrowser::AssetBrowserFavoritesModel(parent))
            , m_delegate(new FavoritesEntryDelegate(this))
        {
            setModel(m_favoritesModel.data());
            setItemDelegate(m_delegate.data());
            setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
            setContextMenuPolicy(Qt::CustomContextMenu);

            m_delegate.data()->SetShowFavoriteIcons(true);

            header()->hide();

            connect(m_delegate.data(), &EntryDelegate::RenameEntry, this, &AssetBrowserFavoritesView::AfterRename);
            connect(this, &QTreeView::customContextMenuRequested, this, &AssetBrowserFavoritesView::OnContextMenu);
        }

        void AssetBrowserFavoritesView::SetSearchWidget(SearchWidget* searchWidget)
        {
            m_favoritesModel.data()->SetSearchWidget(searchWidget);
        }

        void AssetBrowserFavoritesView::selectionChanged([[maybe_unused]] const QItemSelection& selected, [[maybe_unused]] const QItemSelection& deselected)
        {
            if (selected.isEmpty())
            {
                return;
            }

            m_favoritesModel->Select(selected.indexes().at(0));
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

            menu.addAction(
                QObject::tr("Select"),
                [this, selected]()
                {
                    m_favoritesModel->Select(selected);
                });

            menu.addAction(
                QIcon(QStringLiteral(":/Gallery/Favorites.svg")),
                QObject::tr("Remove from Favorites"),
                [favoriteItem]()
                {
                    AssetBrowserFavoriteRequestBus::Broadcast(&AssetBrowserFavoriteRequestBus::Events::RemoveFromFavorites, favoriteItem);
                });

            if (favoriteType == AssetBrowserFavoriteItem::FavoriteType::Search)
            {
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

                m_favoritesModel.data()->SaveFavorites();
            }
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework

#include "AssetBrowser/Favorites/moc_AssetBrowserFavoritesView.cpp"
