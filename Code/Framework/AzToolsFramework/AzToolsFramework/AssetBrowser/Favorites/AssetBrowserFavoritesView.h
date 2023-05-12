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

            void AfterRename(QString newName);

            void SetSearchDisabled(bool disabled);

            AssetBrowserFavoritesModel* GetModel();
        protected:
            // Qt overrides
            void dragMoveEvent(QDragMoveEvent* event) override;

        private:
            QScopedPointer<AzToolsFramework::AssetBrowser::AssetBrowserFavoritesModel> m_favoritesModel;
            QScopedPointer<FavoritesEntryDelegate> m_delegate;

            void OnContextMenu(const QPoint& point);
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
