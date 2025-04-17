/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QStyledItemDelegate>

#include <AzToolsFramework/AssetBrowser/Views/EntryDelegate.h>
#endif


class QWidget;
class QPainter;
class QStyleOptionViewItem;

namespace AzToolsFramework
{
    namespace AssetBrowser
    {

        //! EntryDelegate draws a single item in AssetBrowser.
        class FavoritesEntryDelegate
            : public EntryDelegate
        {
            Q_OBJECT

            void paintAssetBrowserFavoriteSearch(QPainter* painter, int column, const AZStd::string& searchTerm, const QStyleOptionViewItem& option, QStyle* style) const;

        public:
            explicit FavoritesEntryDelegate(QWidget* parent = nullptr);
            ~FavoritesEntryDelegate() override;

            void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
