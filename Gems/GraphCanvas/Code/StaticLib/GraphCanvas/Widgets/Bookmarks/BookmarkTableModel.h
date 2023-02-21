/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <qabstractitemmodel.h>
#include <qitemdelegate.h>
#include <qobject.h>
#include <qsortfilterproxymodel.h>
#include <qregexp.h>

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>

#include <GraphCanvas/Components/Bookmarks/BookmarkBus.h>
#include <GraphCanvas/Editor/EditorTypes.h>
#endif

namespace GraphCanvas
{
    class BookmarkShorcutComboBoxDelegate : public QItemDelegate
    {
        Q_OBJECT
    public:
        BookmarkShorcutComboBoxDelegate(QObject* parent = nullptr);
        ~BookmarkShorcutComboBoxDelegate() = default;

        // QItemDelegate
        QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        void setEditorData(QWidget* editor, const QModelIndex& index) const override;
        void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
        void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        ////

    public Q_SLOTS:

        // QComboBox
        void OnIndexChanged(int index);
        ////

    private:

        bool m_blockShow = false;
        AZStd::vector< QString > m_shortcuts;
    };

    //! Contains all of the information needed ot display all of hte Bookmark information regarding
    //! GraphCanvas
    class BookmarkTableSourceModel
        : public QAbstractTableModel
        , public BookmarkNotificationBus::MultiHandler
        , public BookmarkManagerNotificationBus::Handler
    {
    public:
        enum ColumnDescriptor
        {
            CD_IndexForce = -1,

            CD_Name,
            CD_Shortcut,

            CD_Count
        };
    
        AZ_CLASS_ALLOCATOR(BookmarkTableSourceModel, AZ::SystemAllocator);

        BookmarkTableSourceModel();
        ~BookmarkTableSourceModel();
        
        void SetActiveScene(const AZ::EntityId& sceneId);

        // QAbstractTableModel
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        int columnCount(const QModelIndex& index = QModelIndex()) const override;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        bool setData(const QModelIndex &index, const QVariant &value, int role) override;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
        Qt::ItemFlags flags(const QModelIndex& index) const override;
        ////

        const AZ::EntityId& FindBookmarkForRow(int row) const;
        const AZ::EntityId& FindBookmarkForIndex(const QModelIndex& index) const;
        
        // BookmarkManagerNotifications
        void OnBookmarkAdded(const AZ::EntityId& bookmarkId) override;
        void OnBookmarkRemoved(const AZ::EntityId& bookmarkId) override;
        void OnShortcutChanged(int shortcut, const AZ::EntityId& oldBookmark, const AZ::EntityId& newBookmark) override;
        //// 

        // BookmarkNotificationBus
        void OnBookmarkNameChanged() override;
        void OnBookmarkColorChanged() override;
        ////

    private:

        void CreateBookmarkIcon(const AZ::EntityId& bookmarkId);

        void ClearBookmarks();

        int FindRowForBookmark(const AZ::EntityId& bookmarkId) const;

        AZ::EntityId                  m_activeScene;
        EditorId                      m_activeEditorId;
        AZStd::vector< AZ::EntityId > m_activeBookmarks;

        AZStd::unordered_map< AZ::EntityId, QPixmap* > m_bookmarkIcons;
    };

    class BookmarkTableSortProxyModel
        : public QSortFilterProxyModel
    {
    public:

        AZ_CLASS_ALLOCATOR(BookmarkTableSortProxyModel, AZ::SystemAllocator);

        BookmarkTableSortProxyModel(BookmarkTableSourceModel* sourceModel);
        ~BookmarkTableSortProxyModel() override = default;

        bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

        void SetFilter(const QString& filter);
        void ClearFilter();

    private:

        QString m_filter;
        QRegExp m_filterRegex;
    };
}
