/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/Views/EntryDelegate.h>
#include <AzToolsFramework/AssetBrowser/Favorites/AssetBrowserFavoriteItem.h>
#include <AzToolsFramework/AssetBrowser/Favorites/EntryAssetBrowserFavoriteItem.h>
#include <AzToolsFramework/AssetBrowser/Favorites/SearchAssetBrowserFavoriteItem.h>
#include <AzToolsFramework/AssetBrowser/Favorites/FavoritesEntryDelegate.h>


#include <QApplication>
#include <QPainter>


namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        const int EntrySpacingLeftPixels = 8;
        const int EntryIconMarginLeftPixels = 2;

        FavoritesEntryDelegate::FavoritesEntryDelegate(QWidget* parent)
            : EntryDelegate(parent)
        {
        }

        FavoritesEntryDelegate::~FavoritesEntryDelegate() = default;

        void FavoritesEntryDelegate::paintAssetBrowserFavoriteSearch(
            QPainter* painter, int column, const AZStd::string& searchTerm, const QStyleOptionViewItem& option, QStyle* style) const
        {
            bool isEnabled = (option.state & QStyle::State_Enabled) != 0;
            bool isSelected = (option.state & QStyle::State_Selected) != 0;

            // Draw main entry thumbnail.
            QRect remainingRect(option.rect);
            remainingRect.adjust(EntryIconMarginLeftPixels, 0, 0, 0); // bump it rightwards to give some margin to the icon.

            QSize iconSize(m_iconSize, m_iconSize);
            // Note that the thumbnail might actually be smaller than the row if theres a lot of padding or font size
            // so it needs to center vertically with padding in that case:
            QPoint iconTopLeft(remainingRect.x(), remainingRect.y() + (remainingRect.height() / 2) - (m_iconSize / 2));

            QPalette actualPalette(option.palette);
            if (column == aznumeric_cast<int>(AssetBrowserEntry::Column::Name))
            {
                QIcon icon;
                icon.addFile(":/stylesheet/img/search.svg", iconSize, QIcon::Normal, QIcon::Off);
                icon.paint(painter, QRect(iconTopLeft.x(), iconTopLeft.y(), iconSize.width(), iconSize.height()));
                
                remainingRect.adjust(m_iconSize, 0, 0, 0); // bump it to the right by the size of the thumbnail
                remainingRect.adjust(EntrySpacingLeftPixels, 0, 0, 0); // bump it to the right by the spacing.
            }

            style->drawItemText(
                painter,
                remainingRect,
                option.displayAlignment,
                actualPalette,
                isEnabled,
                searchTerm.data(),
                isSelected ? QPalette::HighlightedText : QPalette::Text);
        }

        void FavoritesEntryDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
        {
            auto data = index.data(AssetBrowserModel::Roles::EntryRole);

            if (data.canConvert<const AssetBrowserFavoriteItem*>())
            {
                auto favorite = qvariant_cast<const AssetBrowserFavoriteItem*>(data);

                if (favorite->GetFavoriteType() == AssetBrowserFavoriteItem::FavoriteType::Search)
                {
                    QStyle* style = option.widget ? option.widget->style() : QApplication::style();

                    // draw the background
                    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter, option.widget);

                    const SearchAssetBrowserFavoriteItem* searchFavorite = reinterpret_cast<const SearchAssetBrowserFavoriteItem*>(favorite);
                    paintAssetBrowserFavoriteSearch(painter, index.column(), searchFavorite->GetName().toUtf8().data(), option, style);
                }
                if (favorite->GetFavoriteType() == AssetBrowserFavoriteItem::FavoriteType::AssetBrowserEntry)
                {
                    QStyle* style = option.widget ? option.widget->style() : QApplication::style();

                    // draw the background
                    if (!index.parent().isValid() && !(option.state & QStyle::State_MouseOver) && !(option.state & QStyle::State_Selected))
                    {
                        painter->fillRect(option.rect, 0x333333);
                    }
                    else
                    {
                        style->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter, option.widget);
                    }

                    const EntryAssetBrowserFavoriteItem* entryFavorite = reinterpret_cast<const EntryAssetBrowserFavoriteItem*>(favorite);
                    paintAssetBrowserEntry(painter, index.column(), entryFavorite->GetEntry(), option);
                }
            }
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework

#include "AssetBrowser/Favorites/moc_FavoritesEntryDelegate.cpp"
