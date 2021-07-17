/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <qapplication.h>
#include <qheaderview.h>
#include <qlineedit.h>
#include <qmessagebox.h>
#include <qmenu.h>
#include <qtableview.h>

#include <GraphCanvas/Widgets/Bookmarks/BookmarkDockWidget.h>
#include <StaticLib/GraphCanvas/Widgets/Bookmarks/ui_BookmarkDockWidget.h>

#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Editor/GraphModelBus.h>
#include <GraphCanvas/Editor/GraphCanvasProfiler.h>
#include <GraphCanvas/Widgets/Bookmarks/BookmarkTableModel.h>
#include <GraphCanvas/Widgets/Resources/Resources.h>

namespace
{
    class DockWidgetBookmarkContextMenu
        : public QMenu
    {
    public:
        DockWidgetBookmarkContextMenu(const AZ::EntityId& graphCanvasGraphId, const AZ::EntityId& bookmarkId)
            : QMenu()
        {
            AZStd::string bookmarkName;
            GraphCanvas::BookmarkRequestBus::EventResult(bookmarkName, bookmarkId, &GraphCanvas::BookmarkRequests::GetBookmarkName);

            QAction* triggerAction = new QAction(QObject::tr("Go to %1").arg(bookmarkName.c_str()), this);
            triggerAction->setToolTip(QObject::tr("Focuses on the selected bookmark"));
            triggerAction->setStatusTip(QObject::tr("Focuses on the selected bookmark"));

            QObject::connect(triggerAction, &QAction::triggered,
                [graphCanvasGraphId, bookmarkId](bool)
            {
                GraphCanvas::BookmarkManagerRequestBus::Event(graphCanvasGraphId, &GraphCanvas::BookmarkManagerRequests::JumpToBookmark, bookmarkId);
            });

            QAction* deleteAction = new QAction(QObject::tr("Delete %1").arg(bookmarkName.c_str()), this);
            deleteAction->setToolTip(QObject::tr("Deletes the selected bookmark from the graph."));
            deleteAction->setStatusTip(QObject::tr("Deletes the selected bookmark from the graph."));

            QObject::connect(deleteAction, &QAction::triggered,
                [bookmarkId](bool)
            {
                GraphCanvas::BookmarkRequestBus::Event(bookmarkId, &GraphCanvas::BookmarkRequests::RemoveBookmark);
            });

            addAction(triggerAction);
            addSeparator();
            addAction(deleteAction);
        }
    };
}

namespace GraphCanvas
{
    ///////////////////////
    // BookmarkDockWidget
    ///////////////////////
    
    BookmarkDockWidget::BookmarkDockWidget(EditorId editorId, QWidget* parent)
        : AzQtComponents::StyledDockWidget(parent)
        , m_ui(new Ui::BookmarkDockWidget())
        , m_editorId(editorId)
        , m_model(nullptr)
    {
        setFocusPolicy(Qt::FocusPolicy::StrongFocus);

        m_filterTimer.setInterval(250);
        m_filterTimer.setSingleShot(true);
        m_filterTimer.stop();

        QObject::connect(&m_filterTimer, &QTimer::timeout, this, &BookmarkDockWidget::UpdateFilter);

        m_ui->setupUi(this);

        m_ui->m_quickFilter->setClearButtonEnabled(true);
        QObject::connect(m_ui->m_quickFilter, &QLineEdit::textChanged, this, &BookmarkDockWidget::OnQuickFilterChanged);

        m_ui->m_quickFilter->setEnabled(false);

        m_model = aznew BookmarkTableSourceModel();
        BookmarkTableRequestBus::Handler::BusConnect(m_model);

        m_proxyModel = aznew GraphCanvas::BookmarkTableSortProxyModel(m_model);

        m_ui->bookmarkTable->setModel(m_proxyModel);
        m_ui->bookmarkTable->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);

        m_ui->bookmarkTable->setItemDelegateForColumn(BookmarkTableSourceModel::CD_Name, aznew GraphCanvas::IconDecoratedNameDelegate(m_ui->bookmarkTable));

        QHeaderView* horizontalHeader = m_ui->bookmarkTable->horizontalHeader();
        horizontalHeader->setSectionResizeMode(BookmarkTableSourceModel::CD_Name, QHeaderView::ResizeMode::Stretch);
        horizontalHeader->setSectionResizeMode(BookmarkTableSourceModel::CD_Shortcut, QHeaderView::ResizeMode::ResizeToContents);

        BookmarkShorcutComboBoxDelegate* shortcutDelegate = new BookmarkShorcutComboBoxDelegate(this);
        m_ui->bookmarkTable->setItemDelegateForColumn(BookmarkTableSourceModel::CD_Shortcut, shortcutDelegate);

        m_ui->bookmarkTable->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(m_ui->bookmarkTable, &QWidget::customContextMenuRequested, this, &BookmarkDockWidget::OnContextMenuRequested);
        connect(m_ui->bookmarkTable->selectionModel(), &QItemSelectionModel::selectionChanged, this, &BookmarkDockWidget::SelectionChanged);

        connect(m_ui->createBookmarkButton, &QPushButton::clicked, this, &BookmarkDockWidget::OnCreateBookmark);
        connect(m_ui->deleteBookmarkButton, &QPushButton::clicked, this, &BookmarkDockWidget::OnDeleteBookmark);

        AssetEditorNotificationBus::Handler::BusConnect(editorId);

        OnActiveGraphChanged(AZ::EntityId());
    }
    
    BookmarkDockWidget::~BookmarkDockWidget()
    {
        GraphCanvas::BookmarkTableRequestBus::Handler::BusDisconnect();
        SceneNotificationBus::Handler::BusDisconnect();
    }

    void BookmarkDockWidget::OnActiveGraphChanged(const GraphId& graphId)
    {
        SceneNotificationBus::Handler::BusDisconnect();

        ClearSelection();
        ClearFilter();

        m_ui->m_quickFilter->setEnabled(graphId.IsValid());
        m_ui->createBookmarkButton->setEnabled(graphId.IsValid());
        m_ui->deleteBookmarkButton->setEnabled(graphId.IsValid());

        m_model->SetActiveScene(graphId);

        m_activeGraphCanvasGraphId = graphId;

        SceneNotificationBus::Handler::BusConnect(m_activeGraphCanvasGraphId);
    }

    void BookmarkDockWidget::ClearSelection()
    {
        m_ui->bookmarkTable->selectionModel()->clearSelection();
    }

    void BookmarkDockWidget::OnSelectionChanged()
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        m_ui->bookmarkTable->clearSelection();
    }

    void BookmarkDockWidget::OnCreateBookmark()
    {
        bool createdAnchor = false;

        AZ::EntityId viewId;
        SceneRequestBus::EventResult(viewId, m_activeGraphCanvasGraphId, &SceneRequests::GetViewId);

        AZ::Vector2 position(0, 0);
        ViewRequestBus::EventResult(position, viewId, &ViewRequests::GetViewSceneCenter);

        BookmarkManagerRequestBus::EventResult(createdAnchor, m_activeGraphCanvasGraphId, &BookmarkManagerRequests::CreateBookmarkAnchor, position, k_findShortcut);

        if (createdAnchor)
        {
            GraphModelRequestBus::Event(m_activeGraphCanvasGraphId, &GraphModelRequests::RequestUndoPoint);
        }
    }

    void BookmarkDockWidget::OnDeleteBookmark()
    {
        QItemSelection itemSelection = m_ui->bookmarkTable->selectionModel()->selection();

        AZStd::vector< AZ::EntityId > removeQueue;
        removeQueue.reserve(itemSelection.size());

        for (const QModelIndex& indexes : itemSelection.indexes())
        {
            QModelIndex sourceIndex = m_proxyModel->mapToSource(indexes);

            AZ::EntityId bookmarkId = m_model->FindBookmarkForIndex(sourceIndex);
            removeQueue.emplace_back(bookmarkId);
        }

        ClearSelection();

        GraphModelRequestBus::Event(m_activeGraphCanvasGraphId, &GraphModelRequests::RequestPushPreventUndoStateUpdate);

        for (const AZ::EntityId& bookmarkId : removeQueue)
        {
            BookmarkRequestBus::Event(bookmarkId, &BookmarkRequests::RemoveBookmark);
        }

        GraphModelRequestBus::Event(m_activeGraphCanvasGraphId, &GraphModelRequests::RequestPopPreventUndoStateUpdate);
        GraphModelRequestBus::Event(m_activeGraphCanvasGraphId, &GraphModelRequests::RequestUndoPoint);
    }

    void BookmarkDockWidget::OnContextMenuRequested(const QPoint& pos)
    {
        QModelIndex index = m_ui->bookmarkTable->indexAt(pos);

        if (index.isValid())
        {
            QModelIndex sourceIndex = m_proxyModel->mapToSource(index);

            AZ::EntityId bookmarkId = m_model->FindBookmarkForIndex(sourceIndex);

            DockWidgetBookmarkContextMenu menu(m_activeGraphCanvasGraphId, bookmarkId);
            menu.exec(m_ui->bookmarkTable->mapToGlobal(pos));
        }
    }


    void BookmarkDockWidget::SelectionChanged(const QItemSelection &selected, const QItemSelection &/*deselected*/)
    {
        if (selected.isEmpty())
        {
            return;
        }

        QModelIndexList indexList = m_ui->bookmarkTable->selectionModel()->selectedIndexes();

        SceneNotificationBus::Handler::BusDisconnect();
        SceneRequestBus::Event(m_activeGraphCanvasGraphId, &SceneRequests::ClearSelection);

        for (const QModelIndex& index : indexList)
        {
            QModelIndex sourceIndex = m_proxyModel->mapToSource(index);
            AZ::EntityId bookmarkId = m_model->FindBookmarkForIndex(sourceIndex);

            SceneMemberUIRequestBus::Event(bookmarkId, &SceneMemberUIRequests::SetSelected, true);
        }

        SceneNotificationBus::Handler::BusConnect(m_activeGraphCanvasGraphId);
    }

    void BookmarkDockWidget::OnQuickFilterChanged(const QString &text)
    {
        if(text.isEmpty())
        {
            //If filter was cleared, update immediately
            UpdateFilter();
            return;
        }
        m_filterTimer.stop();
        m_filterTimer.start();
    }

    void BookmarkDockWidget::UpdateFilter()
    {
        m_proxyModel->SetFilter(m_ui->m_quickFilter->text());
    }

    void BookmarkDockWidget::ClearFilter()
    {
        {
            QSignalBlocker blocker(m_ui->m_quickFilter);
            m_ui->m_quickFilter->setText("");
        }

        UpdateFilter();
    }

#include <StaticLib/GraphCanvas/Widgets/Bookmarks/moc_BookmarkDockWidget.cpp>
}
