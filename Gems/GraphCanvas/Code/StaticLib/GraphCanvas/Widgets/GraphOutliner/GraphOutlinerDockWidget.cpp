/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GraphCanvas/Widgets/GraphOutliner/GraphOutlinerDockWidget.h>
#include <QHeaderView>
#include <GraphCanvas/Components/VisualBus.h>

namespace GraphCanvas
{
    ///////////////////////////
    // GraphOutlinerDockWidget
    ///////////////////////////

    GraphOutlinerDockWidget::GraphOutlinerDockWidget(EditorId editorId, QWidget* parent)
        : AzQtComponents::StyledDockWidget(parent)
        , m_editorId(editorId)
    {
        CreateUI();

        m_model = aznew NodeTableSourceModel();
        m_proxyModel = aznew NodeTableSortProxyModel(m_model);

        m_nodelistTable->setModel(m_proxyModel);
        m_nodelistTable->horizontalHeader()->setSectionResizeMode(NodeTableSourceModel::CD_Name, QHeaderView::ResizeMode::Stretch);
        m_nodelistTable->horizontalHeader()->setVisible(false);
        m_nodelistTable->verticalHeader()->setVisible(false);

        m_filterTimer.setInterval(250);
        m_filterTimer.setSingleShot(true);
        m_filterTimer.stop();

        connect(&m_filterTimer, &QTimer::timeout, this, &GraphOutlinerDockWidget::UpdateFilter);
        connect(m_quickFilter, &QLineEdit::textChanged, this, &GraphOutlinerDockWidget::OnQuickFilterChanged);
        connect(m_nodelistTable->selectionModel(), &QItemSelectionModel::selectionChanged, this, &GraphOutlinerDockWidget::SelectionChanged);
        connect(m_nodelistTable, &QTableView::doubleClicked, this, &GraphOutlinerDockWidget::OnDoubleClicked);
        AssetEditorNotificationBus::Handler::BusConnect(editorId);

        OnActiveGraphChanged(AZ::EntityId());
    }
    GraphOutlinerDockWidget::~GraphOutlinerDockWidget()
    {
        AssetEditorNotificationBus::Handler::BusDisconnect(m_editorId);
        SceneNotificationBus::Handler::BusDisconnect();
    }

    void GraphOutlinerDockWidget::CreateUI()
    {
        QWidget* dockWidgetContents;
        QScrollArea* scrollArea;
        QWidget* scrollAreaWidgetContents;
        QVBoxLayout* verticalLayout;

        dockWidgetContents = new QWidget();

        scrollArea = new QScrollArea(dockWidgetContents);
        scrollArea->setFrameShape(QFrame::NoFrame);
        scrollArea->setWidgetResizable(true);

        scrollAreaWidgetContents = new QWidget();
        scrollAreaWidgetContents->setGeometry(QRect(0, 0, 262, 521));

        verticalLayout = new QVBoxLayout(dockWidgetContents);
        verticalLayout->setSpacing(0);
        verticalLayout->setContentsMargins(5, 5, 5, 5);

        m_quickFilter = new AzQtComponents::SearchLineEdit(dockWidgetContents);
        m_quickFilter->setPlaceholderText("Input node name...");
        m_quickFilter->setClearButtonEnabled(true);
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(m_quickFilter->sizePolicy().hasHeightForWidth());
        m_quickFilter->setSizePolicy(sizePolicy);
        m_quickFilter->setEnabled(false);

        m_nodelistTable = new QTableView(dockWidgetContents);
        m_nodelistTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        m_nodelistTable->setEditTriggers(
            QAbstractItemView::AnyKeyPressed | QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
        m_nodelistTable->setAlternatingRowColors(true);
        m_nodelistTable->setSelectionMode(QAbstractItemView::SingleSelection);
        m_nodelistTable->setSelectionBehavior(QAbstractItemView::SelectRows);

        scrollArea->setWidget(scrollAreaWidgetContents);
        verticalLayout->addWidget(m_quickFilter);
        verticalLayout->addWidget(m_nodelistTable);

        this->setWindowTitle("Graph Outliner");
        this->setWidget(dockWidgetContents);

    }

    void GraphOutlinerDockWidget::OnDoubleClicked(const QModelIndex& index)
    {
        m_model->JumpToNodeArea(m_proxyModel->mapToSource(index));
    }

    void GraphOutlinerDockWidget::OnActiveGraphChanged([[maybe_unused]] const GraphId& graphId)
    {
        SceneNotificationBus::Handler::BusDisconnect();

        m_nodelistTable->selectionModel()->clear();
        ClearFilter();

        m_quickFilter->setEnabled(graphId.IsValid());
        m_model->SetActiveScene(graphId);
        m_activeGraphCanvasGraphId = graphId;

        SceneNotificationBus::Handler::BusConnect(m_activeGraphCanvasGraphId);
    }

    void GraphOutlinerDockWidget::OnContextMenuRequested([[maybe_unused]] const QPoint& pos)
    {
    }
    void GraphOutlinerDockWidget::SelectionChanged(const QItemSelection& selected, [[maybe_unused]] const QItemSelection& deselected)
    {
        if (selected.isEmpty())
        {
            return;
        }
        auto index = m_nodelistTable->selectionModel()->selectedIndexes().at(0);
        AZ::EntityId nodeId = m_model->FindNodeByIndex(m_proxyModel->mapToSource(index));

        SceneNotificationBus::Handler::BusDisconnect();
        SceneRequestBus::Event(m_activeGraphCanvasGraphId, &SceneRequests::ClearSelection);
        SceneMemberUIRequestBus::Event(nodeId, &SceneMemberUIRequests::SetSelected, true);
        SceneNotificationBus::Handler::BusConnect(m_activeGraphCanvasGraphId);
    }

    void GraphOutlinerDockWidget::OnSelectionChanged()
    {
        m_nodelistTable->selectionModel()->clear();
    }

    void GraphOutlinerDockWidget::OnQuickFilterChanged([[maybe_unused]] const QString& text)
    {
        if (text.isEmpty())
        {
            // If filter was cleared, update immediately
            UpdateFilter();
            return;
        }
        m_filterTimer.stop();
        m_filterTimer.start();
    }

    void GraphOutlinerDockWidget::UpdateFilter()
    {
        m_proxyModel->SetFilter(m_quickFilter->text());
    }

    void GraphOutlinerDockWidget::ClearFilter()
    {
        {
            QSignalBlocker blocker(m_quickFilter);
            m_quickFilter->setText("");
        }

        UpdateFilter();
    }
}
