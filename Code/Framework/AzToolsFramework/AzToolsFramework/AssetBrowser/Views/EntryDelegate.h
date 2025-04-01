/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/function/function_fwd.h>
#include <AzToolsFramework/Thumbnails/Thumbnail.h>
AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // 4251: class 'QScopedPointer<QBrushData,QBrushDataPointerDeleter>' needs to have dll-interface to be used by clients of class 'QBrush'
                                                               // 4800: 'uint': forcing value to bool 'true' or 'false' (performance warning)
#include <QStyledItemDelegate>
#endif
AZ_POP_DISABLE_WARNING

class QWidget;
class QPainter;
class QStyleOptionViewItem;
class QModelIndex;

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        //! Type of branch icon the delegate should paint.
        enum EntryBranchType
        {
            First,
            Middle,
            Last,
            OneChild,
            Count
        };

        class AssetBrowserFilterModel;
        class AssetBrowserTreeView;
        class AssetBrowserEntry;

        //! EntryDelegate draws a single item in AssetBrowser.
        class EntryDelegate
            : public QStyledItemDelegate
        {
            Q_OBJECT
        public:
            explicit EntryDelegate(QWidget* parent = nullptr);
            ~EntryDelegate() override;

            void paintAssetBrowserEntry(QPainter* painter, int column, const AssetBrowserEntry* entry, const QStyleOptionViewItem& option) const;

            QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
            void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
            QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
            //! Set whether to show source control icons, this is still temporary mainly to support existing functionality of material browser
            void SetShowSourceControlIcons(bool showSourceControl);
            void SetShowFavoriteIcons(bool showFavoriteIcons);
            void SetSearchString(const QString& searchString);

        signals:
            void RenameEntry(const QString& value) const;

        protected:
            int m_iconSize;
            bool m_showSourceControl = false;
            bool m_showFavoriteIcons = false;
            //! Draw a thumbnail and return its width
            int DrawThumbnail(QPainter* painter, const QPoint& point, const QSize& size, const AssetBrowserEntry* entry) const;
            QString m_searchString;
        };

        //! SearchEntryDelegate draws a single item in AssetBrowserListView.
        class SearchEntryDelegate
            : public EntryDelegate
        {
            Q_OBJECT
        public:
            explicit SearchEntryDelegate(QWidget* parent = nullptr);
            void Init();
            void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

        private:
            void LoadBranchPixMaps();
            void DrawBranchPixMap(EntryBranchType branchType, QPainter* painter, const QPoint& point, const QSize& size) const;

        private:
            AssetBrowserFilterModel* m_assetBrowserFilerModel;
            QMap<EntryBranchType, QPixmap> m_branchIcons;
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
