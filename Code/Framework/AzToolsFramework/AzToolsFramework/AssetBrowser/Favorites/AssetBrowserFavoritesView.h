/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)

#include <AzCore/Memory/SystemAllocator.h>

#include <AzToolsFramework/UI/UICore/QTreeViewStateSaver.hxx>

#include <QString>
#endif

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserFavoritesModel;
        class AssetBrowserTreeView;
        class FavoritesEntryDelegate;
        class SearchWidget;

        class AssetBrowserFavoritesView
            : public QTreeViewWithStateSaving
        {
            Q_OBJECT
            
        public:
            explicit AssetBrowserFavoritesView(QWidget* parent = nullptr);

            void SetSearchWidget(SearchWidget* searchWidget);

            void AddSelectedEntriesToFavorites();

            void AfterRename(QString newName);

            void SetSearchDisabled(bool disabled);

            AssetBrowserFavoritesModel* GetModel();

        Q_SIGNALS:
            void setFavoritesWindowHeight(int height);

        protected:
            // Qt overrides
            void dragMoveEvent(QDragMoveEvent* event) override;
            void mouseDoubleClickEvent(QMouseEvent* event) override;
            void resizeEvent(QResizeEvent* event) override;

            void collapsed(const QModelIndex& index);
            void expanded(const QModelIndex& index);

            void drawBranches(QPainter* painter, const QRect& rect, const QModelIndex& index) const override;

        protected Q_SLOTS:
            void SelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
            void ItemClicked(const QModelIndex& index);

        private:
            QScopedPointer<AzToolsFramework::AssetBrowser::AssetBrowserFavoritesModel> m_favoritesModel;
            QScopedPointer<FavoritesEntryDelegate> m_delegate;
            int m_currentHeight = 0;

            void OnContextMenu(const QPoint& point);

            void NotifySelection(const QItemSelection& selected);
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
