/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <QApplication>
#include <QComboBox>
#include <QMessageBox>

#include <GraphCanvas/Widgets/Bookmarks/BookmarkTableModel.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/StyleBus.h>
#include <GraphCanvas/Widgets/GraphCanvasGraphicsView/GraphCanvasGraphicsView.h>

namespace GraphCanvas
{
    ////////////////////////////////////
    // BookmarkShorcutComboBoxDelegate
    ////////////////////////////////////

    BookmarkShorcutComboBoxDelegate::BookmarkShorcutComboBoxDelegate(QObject* parent)
        : QItemDelegate(parent)
        , m_shortcuts({ {" ", "1","2","3","4","5","6","7","8","9"} })
    {
    }

    QWidget* BookmarkShorcutComboBoxDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& /*option*/, const QModelIndex& /*index*/) const
    {
        QComboBox* comboBox = new QComboBox(parent);

        for (const QString& item : m_shortcuts)
        {
            comboBox->addItem(item);
        }

        QObject::connect(comboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &BookmarkShorcutComboBoxDelegate::OnIndexChanged);

        return comboBox;
    }

    void BookmarkShorcutComboBoxDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
    {
        QComboBox* comboBox = qobject_cast<QComboBox*>(editor);

        if (comboBox)
        {
            QSignalBlocker blocker(comboBox);
            int value = index.model()->data(index, Qt::EditRole).toInt();

            if (value == k_unusedShortcut)
            {
                value = 0;
            }
            
            comboBox->setCurrentIndex(value);
            
            if (!m_blockShow)
            {
                comboBox->showPopup();
            }
        }
    }

    void BookmarkShorcutComboBoxDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
    {
        QComboBox *comboBox = qobject_cast<QComboBox*>(editor);

        if (comboBox)
        {
            int shortcut = comboBox->currentIndex();

            if (shortcut == 0)
            {
                shortcut = k_unusedShortcut;
            }

            model->setData(index, shortcut, Qt::EditRole);
        }
    }

    void BookmarkShorcutComboBoxDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& /*index*/) const
    {
        editor->setGeometry(option.rect);
    }

    void BookmarkShorcutComboBoxDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
#if (QT_VERSION < QT_VERSION_CHECK(5, 11, 0))
        QStyleOptionViewItemV4 myOption = option;
#else
        QStyleOptionViewItem myOption = option;
#endif
        myOption.text = index.model()->data(index, Qt::DisplayRole).toString();
        QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &myOption, painter);
    }

    void BookmarkShorcutComboBoxDelegate::OnIndexChanged(int /*index*/)
    {
        QComboBox* comboBox = qobject_cast<QComboBox*>(sender());

        if (comboBox)
        {
            m_blockShow = true;
            emit commitData(comboBox);
            m_blockShow = false;
        }
    }

    /////////////////////////////
    // BookmarkTableSourceModel
    /////////////////////////////
    BookmarkTableSourceModel::BookmarkTableSourceModel()
        : QAbstractTableModel(nullptr)
    {
    }

    BookmarkTableSourceModel::~BookmarkTableSourceModel()
    {
        ClearBookmarks();
    }
    
    void BookmarkTableSourceModel::SetActiveScene(const AZ::EntityId& sceneId)
    {
        m_activeScene = sceneId;
        SceneRequestBus::EventResult(m_activeEditorId, m_activeScene, &SceneRequests::GetEditorId);

        BookmarkNotificationBus::MultiHandler::BusDisconnect();

        layoutAboutToBeChanged();
        ClearBookmarks();
        
        SceneBookmarkRequestBus::EnumerateHandlersId(sceneId, [this](SceneBookmarkRequests* bookmarkRequest)
        {
            m_activeBookmarks.push_back(bookmarkRequest->GetBookmarkId());
            return true;
        });

        for (const AZ::EntityId& bookmarkId : m_activeBookmarks)
        {
            CreateBookmarkIcon(bookmarkId);
            BookmarkNotificationBus::MultiHandler::BusConnect(bookmarkId);
        }
        
        layoutChanged();

        BookmarkManagerNotificationBus::Handler::BusDisconnect();
        BookmarkManagerNotificationBus::Handler::BusConnect(sceneId);
    }

    int BookmarkTableSourceModel::rowCount(const QModelIndex& parent) const
    {
        (void)parent;
        return static_cast<int>(m_activeBookmarks.size());
    }
    
    int BookmarkTableSourceModel::columnCount(const QModelIndex& /*index*/) const
    {
        return CD_Count;
    }

    QVariant BookmarkTableSourceModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid())
        {
            return QVariant();
        }
        
        const AZ::EntityId& bookmarkId = FindBookmarkForIndex(index);
        
        switch (role)
        {
            case Qt::DisplayRole:
            {
                if (index.column() == CD_Name)
                {
                    AZStd::string name;
                    BookmarkRequestBus::EventResult(name, bookmarkId, &BookmarkRequests::GetBookmarkName);
                    return QString(name.c_str());
                }
                else if (index.column() == CD_Shortcut)
                {
                    int shortcut = k_unusedShortcut;
                    
                    BookmarkRequestBus::EventResult(shortcut, bookmarkId, &BookmarkRequests::GetShortcut);
                    
                    if (shortcut != k_unusedShortcut)
                    {
                        return QString("%1").arg(shortcut);
                    }
                    else
                    {
                        return QString("");
                    }
                }
            }
            break;
            case Qt::DecorationRole:
            {
                if (index.column() == CD_Name)
                {
                    auto bookmarkIter = m_bookmarkIcons.find(bookmarkId);

                    if (bookmarkIter != m_bookmarkIcons.end())
                    {
                        QPixmap* icon = bookmarkIter->second;

                        if (icon)
                        {
                            return *icon;
                        }
                    }
                }
            }
            break;
            case Qt::EditRole:
            {
                if (index.column() == CD_Name)
                {
                    AZStd::string name;
                    BookmarkRequestBus::EventResult(name, bookmarkId, &BookmarkRequests::GetBookmarkName);
                    return QString(name.c_str());
                }
                else
                {
                    int shortcut = k_unusedShortcut;
                    BookmarkRequestBus::EventResult(shortcut, bookmarkId, &BookmarkRequests::GetShortcut);
                    return shortcut;
                }
            }
            break;
            default:
                break;
        }

        return QVariant();
    }

    bool BookmarkTableSourceModel::setData(const QModelIndex& index, const QVariant& value, int role)
    {
        if (index.column() == CD_Name)
        {
            if (role == Qt::EditRole)
            {
                QString bookmarkName = value.toString();

                const AZ::EntityId& bookmarkId = FindBookmarkForIndex(index);

                BookmarkRequestBus::Event(bookmarkId, &BookmarkRequests::SetBookmarkName, bookmarkName.toStdString().c_str());
                GraphCanvas::GraphModelRequestBus::Event(m_activeScene, &GraphCanvas::GraphModelRequests::RequestUndoPoint);

                return true;
            }
        }
        else if (index.column() == CD_Shortcut)
        {
            if (role == Qt::EditRole)
            {
                bool postUndo = false;

                {
                    GraphCanvas::ScopedGraphUndoBlocker undoBlocker(m_activeScene);

                    int shortcut = value.toInt();

                    const AZ::EntityId& bookmarkId = FindBookmarkForIndex(index);

                    AZ::EntityId existingBookmark;
                    BookmarkManagerRequestBus::EventResult(existingBookmark, m_activeScene, &BookmarkManagerRequests::FindBookmarkForShortcut, shortcut);

                    if (existingBookmark.IsValid() && existingBookmark != bookmarkId)
                    {
                        AZStd::string bookmarkName;
                        BookmarkRequestBus::EventResult(bookmarkName, existingBookmark, &BookmarkRequests::GetBookmarkName);

                        AZ::EntityId viewId;
                        SceneRequestBus::EventResult(viewId, m_activeScene, &SceneRequests::GetViewId);

                        GraphCanvasGraphicsView* graphicsView = nullptr;
                        ViewRequestBus::EventResult(graphicsView, viewId, &ViewRequests::AsGraphicsView);

                        QMessageBox::StandardButton response = QMessageBox::StandardButton::No;
                        response = QMessageBox::question(graphicsView, QString("Bookmarking Conflict"), QString("Bookmark (%1) already registered with shortcut (%2).\nProceed with remapping and remove shortcut?").arg(bookmarkName.c_str()).arg(shortcut), QMessageBox::StandardButton::Yes | QMessageBox::No);

                        if (response == QMessageBox::StandardButton::No)
                        {
                            return false;
                        }
                        else if (response == QMessageBox::StandardButton::Yes)
                        {
                            BookmarkRequestBus::Event(existingBookmark, &BookmarkRequests::RemoveBookmark);
                            postUndo = true;
                        }
                    }

                    if (existingBookmark != bookmarkId)
                    {
                        BookmarkManagerRequestBus::Event(m_activeScene, &BookmarkManagerRequests::RequestShortcut, bookmarkId, shortcut);
                        postUndo = true;
                    }
                }

                if (postUndo)
                {
                    GraphCanvas::GraphModelRequestBus::Event(m_activeScene, &GraphCanvas::GraphModelRequests::RequestUndoPoint);
                }

                return true;
            }
        }

        return false;
    }
    
    QVariant BookmarkTableSourceModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (role == Qt::DisplayRole)
        {
            if (orientation == Qt::Horizontal)
            {
                switch (section)
                {
                case CD_Name:
                    return QString("Name");
                case CD_Shortcut:
                    return QString("Shortcut");
                default:
                    AZ_Assert(false, "Unknown section index %i", section);
                    break;
                }
            }
        }

        return QVariant();
    }

    Qt::ItemFlags BookmarkTableSourceModel::flags(const QModelIndex& index) const
    {
        if (!index.isValid())
        {
            return Qt::ItemFlags();
        }

        Qt::ItemFlags flags = Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable;

        if (index.column() == CD_Name
            || index.column() == CD_Shortcut)
        {
            flags |= Qt::ItemFlag::ItemIsEditable;
        }

        return flags;
    }
    
    const AZ::EntityId& BookmarkTableSourceModel::FindBookmarkForRow(int row) const
    {
        if (row < 0 || row >= m_activeBookmarks.size())
        {
            static const AZ::EntityId k_entityId;
            return k_entityId;
        }
        
        return m_activeBookmarks[row];
    }
    
    const AZ::EntityId& BookmarkTableSourceModel::FindBookmarkForIndex(const QModelIndex& index) const
    {
        if (!index.isValid())
        {
            static const AZ::EntityId k_entityId;
            return k_entityId;
        }
        
        return FindBookmarkForRow(index.row());
    }
    
    void BookmarkTableSourceModel::OnBookmarkAdded(const AZ::EntityId& bookmarkId)
    {
        BookmarkNotificationBus::MultiHandler::BusConnect(bookmarkId);
        BookmarkTableRequestBus::Event(this, &BookmarkTableRequests::ClearSelection);

        beginInsertRows(QModelIndex(), static_cast<int>(m_activeBookmarks.size()), static_cast<int>(m_activeBookmarks.size()));
        m_activeBookmarks.push_back(bookmarkId);
        CreateBookmarkIcon(bookmarkId);
        endInsertRows();
    }
    
    void BookmarkTableSourceModel::OnBookmarkRemoved(const AZ::EntityId& bookmarkId)
    {
        int row = FindRowForBookmark(bookmarkId);

        if (row >= 0)
        {
            BookmarkNotificationBus::MultiHandler::BusDisconnect(bookmarkId);
            BookmarkTableRequestBus::Event(this, &BookmarkTableRequests::ClearSelection);
            beginRemoveRows(QModelIndex(), row, row);
            m_activeBookmarks.erase(AZStd::next(m_activeBookmarks.begin(), row));

            auto bookmarkIter = m_bookmarkIcons.find(bookmarkId);

            if (bookmarkIter != m_bookmarkIcons.end())
            {
                delete bookmarkIter->second;
                m_bookmarkIcons.erase(bookmarkIter);
            }

            endRemoveRows();
        }
    }

    void BookmarkTableSourceModel::OnShortcutChanged(int /*shortcut*/, const AZ::EntityId& oldBookmark, const AZ::EntityId& newBookmark)
    {
        int row = FindRowForBookmark(oldBookmark);

        if (row >= 0)
        {
            dataChanged(createIndex(row, 0), createIndex(row, CD_Count - 1));
        }

        row = FindRowForBookmark(newBookmark);

        if (row >= 0)
        {
            dataChanged(createIndex(row, 0), createIndex(row, CD_Count - 1));
        }
    }
    
    void BookmarkTableSourceModel::OnBookmarkNameChanged()
    {
        AZ::EntityId bookmarkId = (*BookmarkNotificationBus::GetCurrentBusId());
        int row = FindRowForBookmark(bookmarkId);

        if (row >= 0)
        {
            dataChanged(createIndex(row, CD_Name), createIndex(row, CD_Name));
        }
    }

    void BookmarkTableSourceModel::OnBookmarkColorChanged()
    {
        AZ::EntityId bookmarkId = (*BookmarkNotificationBus::GetCurrentBusId());

        CreateBookmarkIcon(bookmarkId);

        int row = FindRowForBookmark(bookmarkId);

        if (row >= 0)
        {
            dataChanged(createIndex(row, CD_Name), createIndex(row, CD_Name));
        }
    }

    void BookmarkTableSourceModel::CreateBookmarkIcon(const AZ::EntityId& bookmarkId)
    {
        QColor color;
        BookmarkRequestBus::EventResult(color, bookmarkId, &BookmarkRequests::GetBookmarkColor);

        auto bookmarkIter = m_bookmarkIcons.find(bookmarkId);

        if (bookmarkIter != m_bookmarkIcons.end())
        {
            delete bookmarkIter->second;
            m_bookmarkIcons.erase(bookmarkIter);
        }

        QPixmap* pixmap = nullptr;
        StyleManagerRequestBus::EventResult(pixmap, m_activeEditorId, &StyleManagerRequests::CreateIcon, color, "BookmarkTypeIcon");

        if (pixmap)
        {
            m_bookmarkIcons[bookmarkId] = pixmap;
        }
    }

    void BookmarkTableSourceModel::ClearBookmarks()
    {
        m_activeBookmarks.clear();

        for (auto mapPair : m_bookmarkIcons)
        {
            delete mapPair.second;
        }

        m_bookmarkIcons.clear();
    }

    int BookmarkTableSourceModel::FindRowForBookmark(const AZ::EntityId& bookmarkId) const
    {
        int row = -1;

        for (int i = 0; i < m_activeBookmarks.size(); ++i)
        {
            if (m_activeBookmarks[i] == bookmarkId)
            {
                row = i;
                break;
            }
        }

        return row;
    }

    ////////////////////////////////
    // BookmarkTableSortProxyModel
    ////////////////////////////////

    BookmarkTableSortProxyModel::BookmarkTableSortProxyModel(BookmarkTableSourceModel* sourceModel)
    {
        setSourceModel(sourceModel);
    }

    bool BookmarkTableSortProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
    {
        QAbstractItemModel* model = sourceModel();
        QModelIndex index = model->index(sourceRow, 0, sourceParent);

        if (m_filter.isEmpty())
        {
            return true;
        }

        QVariant displayData = model->data(index, Qt::DisplayRole);
        QString stringName = displayData.toString();

        return stringName.lastIndexOf(m_filterRegex) >= 0;
    }

    void BookmarkTableSortProxyModel::SetFilter(const QString& filter)
    {
        m_filter = filter;
        m_filterRegex = QRegExp(m_filter, Qt::CaseInsensitive);

        invalidateFilter();
    }

    void BookmarkTableSortProxyModel::ClearFilter()
    {
        SetFilter("");
    }

#include <StaticLib/GraphCanvas/Widgets/Bookmarks/moc_BookmarkTableModel.cpp>
}
