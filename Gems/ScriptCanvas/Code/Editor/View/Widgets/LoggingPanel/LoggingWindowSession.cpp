/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QScrollBar>
#include <QGraphicsItem>
#include <QScopedValueRollback>

#include <GraphCanvas/Components/Nodes/NodeTitleBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Widgets/StyledItemDelegates/IconDecoratedNameDelegate.h>
#include <GraphCanvas/Utils/GraphUtils.h>

#include <Editor/View/Widgets/AssetGraphSceneDataBus.h>
#include <Editor/View/Widgets/LoggingPanel/LoggingWindowSession.h>

#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/GraphCanvas/MappingBus.h>
#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>

namespace ScriptCanvasEditor
{
    /////////////////////////////
    // LoggingWindowFilterModel
    /////////////////////////////

    bool LoggingWindowFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
    {
        if (m_logFilter.IsEmpty())
        {
            return true;
        }

        QAbstractItemModel* model = sourceModel();
        QModelIndex index = model->index(sourceRow, 0, sourceParent);

        DebugLogTreeItem* treeItem = static_cast<DebugLogTreeItem*>(index.internalPointer());

        if (treeItem)
        {
            return treeItem->MatchesFilter(m_logFilter);
        }

        return false;
    }

    void LoggingWindowFilterModel::SetFilter(const QString& filter)
    {
        m_filter = filter;
        m_logFilter.m_filter = QRegExp(m_filter, Qt::CaseInsensitive);

        invalidateFilter();
    }

    void LoggingWindowFilterModel::ClearFilter()
    {
        SetFilter("");
    }

    bool LoggingWindowFilterModel::HasFilter() const
    {
        return !m_filter.isEmpty();
    }

    /////////////////////////
    // LoggingWindowSession
    /////////////////////////

    LoggingWindowSession::LoggingWindowSession(QWidget* parentWidget)
        : QWidget(parentWidget)
        , m_ui(new Ui::LoggingWindowSession())
        , m_clearSelectionOnSceneSelectionChange(true)
        , m_scrollToBottom(true)
        , m_debugRoot(nullptr)
        , m_treeModel(nullptr)
        , m_filterModel(nullptr)
    {
        m_ui->setupUi(this);

        QObject::connect(m_ui->captureButton, &QToolButton::clicked, this, &LoggingWindowSession::OnCaptureButtonPressed);

        QObject::connect(m_ui->expandAll, &QToolButton::clicked, this, &LoggingWindowSession::OnExpandAll);
        QObject::connect(m_ui->collapseAll, &QToolButton::clicked, this, &LoggingWindowSession::OnCollapseAll);

        QObject::connect(m_ui->targetSelector, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &LoggingWindowSession::OnTargetChanged);

        QObject::connect(m_ui->logTree->verticalScrollBar(), &QScrollBar::valueChanged, this, &LoggingWindowSession::OnLogScrolled);
        QObject::connect(m_ui->logTree, &QTreeView::expanded, this, &LoggingWindowSession::OnLogItemExpanded);
        QObject::connect(m_ui->logTree->verticalScrollBar(), &QScrollBar::rangeChanged, this, &LoggingWindowSession::OnLogRangeChanged);

        QObject::connect(m_ui->filterWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged, this, &LoggingWindowSession::OnSearchFilterChanged);
        m_ui->filterWidget->SetFilterInputInterval(AZStd::chrono::milliseconds(250));
        
        m_ui->logTree->setMouseTracking(true);
        QObject::connect(m_ui->logTree, &QTreeView::clicked, this, &LoggingWindowSession::OnLogClicked);

        QObject::connect(m_ui->logTree, &QTreeView::doubleClicked, this, &LoggingWindowSession::OnLogDoubleClicked);

        m_focusDelayTimer.setInterval(125);
        m_focusDelayTimer.setSingleShot(true);

        QObject::connect(&m_focusDelayTimer, &QTimer::timeout, this, &LoggingWindowSession::HandleQueuedFocus);

        GraphCanvas::AssetEditorNotificationBus::Handler::BusConnect(ScriptCanvasEditor::AssetEditorId);

        AZ::EntityId graphCanvasId;
        GeneralRequestBus::BroadcastResult(graphCanvasId, &GeneralRequests::GetActiveGraphCanvasGraphId);

        OnActiveGraphChanged(graphCanvasId);
    }

    LoggingWindowSession::~LoggingWindowSession()
    {

    }

    const LoggingDataId& LoggingWindowSession::GetDataId() const
    {
        return m_loggingDataId;
    }

    void LoggingWindowSession::ClearFilter()
    {
        m_ui->filterWidget->ClearTextFilter();
    }

    void LoggingWindowSession::OnActiveGraphChanged(const AZ::EntityId& graphId)
    {
        ClearLoggingSelection();

        if (GraphCanvas::SceneNotificationBus::Handler::BusIsConnected())
        {
            GraphCanvas::SceneNotificationBus::Handler::BusDisconnect();
        }

        if (graphId.IsValid())
        {
            GraphCanvas::SceneNotificationBus::Handler::BusConnect(graphId);
        }
    }

    void LoggingWindowSession::OnSelectionChanged()
    {
        ClearLoggingSelection();
    }

    void LoggingWindowSession::RegisterTreeRoot(DebugLogRootItem* debugRoot)
    {
        m_debugRoot = debugRoot;
        m_treeModel = aznew GraphCanvas::GraphCanvasTreeModel(debugRoot, this);
        m_filterModel = aznew LoggingWindowFilterModel();

        m_filterModel->setSourceModel(m_treeModel);

        m_ui->logTree->setModel(m_filterModel);

        m_ui->logTree->header()->setStretchLastSection(false);
        m_ui->logTree->header()->setSectionResizeMode(DebugLogTreeItem::Column::NodeName, QHeaderView::ResizeMode::Stretch);
        m_ui->logTree->header()->setSectionResizeMode(DebugLogTreeItem::Column::Input, QHeaderView::ResizeMode::Stretch);
        m_ui->logTree->header()->setSectionResizeMode(DebugLogTreeItem::Column::Output, QHeaderView::ResizeMode::Stretch);

        m_ui->logTree->header()->setSectionResizeMode(DebugLogTreeItem::Column::TimeStep, QHeaderView::ResizeMode::Fixed);
        m_ui->logTree->header()->resizeSection(DebugLogTreeItem::Column::TimeStep, 75);

        m_ui->logTree->header()->setSectionResizeMode(DebugLogTreeItem::Column::ScriptName, QHeaderView::ResizeMode::Fixed);
        m_ui->logTree->header()->resizeSection(DebugLogTreeItem::Column::ScriptName, 150);

        m_ui->logTree->header()->setSectionResizeMode(DebugLogTreeItem::Column::SourceEntity, QHeaderView::ResizeMode::Fixed);
        m_ui->logTree->header()->resizeSection(DebugLogTreeItem::Column::SourceEntity, 200);

        m_ui->logTree->setItemDelegateForColumn(DebugLogTreeItem::Column::NodeName, aznew GraphCanvas::IconDecoratedNameDelegate(this));

        QObject::connect(m_ui->logTree->selectionModel(), &QItemSelectionModel::selectionChanged, this, &LoggingWindowSession::OnLogSelectionChanged);
    }

    void LoggingWindowSession::SetDataId(const LoggingDataId& loggingDataId)
    {
        if (!m_loggingDataId.IsValid())
        {
            m_loggingDataId = loggingDataId;
        }
    }

    void LoggingWindowSession::OnExpandAll()
    {
        m_ui->logTree->expandAll();

        ScrollToSelection();
    }

    void LoggingWindowSession::OnCollapseAll()
    {
        m_ui->logTree->collapseAll();

        ScrollToSelection();
    }

    void LoggingWindowSession::OnSearchFilterChanged(const QString& filterString)
    {
        m_filterModel->SetFilter(filterString);
    }

    void LoggingWindowSession::OnLogScrolled(int value)
    {
        if (m_ui->logTree->verticalScrollBar()->isEnabled())
        {
            if (m_ui->logTree->verticalScrollBar()->maximum() == value)
            {
                m_scrollToBottom = true;
            }
            else
            {
                m_scrollToBottom = false;
            }
        }
        else
        {
            m_scrollToBottom = true;
        }
    }

    void LoggingWindowSession::OnLogItemExpanded([[maybe_unused]] const QModelIndex& modelIndex)
    {
        m_scrollToBottom = false;
    }

    void LoggingWindowSession::OnLogRangeChanged([[maybe_unused]] int min, int max)
    {
        if (m_scrollToBottom)
        {
            m_ui->logTree->verticalScrollBar()->setValue(max);
        }

        if (!m_ui->logTree->verticalScrollBar()->isEnabled())
        {
            m_scrollToBottom = true;
        }
    }

    void LoggingWindowSession::OnLogClicked(const QModelIndex& modelIndex)
    {
        if (modelIndex.column() == DebugLogTreeItem::Column::ScriptName)
        {
            QModelIndex sourceIndex = m_filterModel->mapToSource(modelIndex);
            DebugLogTreeItem* treeItem = static_cast<DebugLogTreeItem*>(sourceIndex.internalPointer());

            if (ExecutionLogTreeItem* executionItem = azrtti_cast<ExecutionLogTreeItem*>(treeItem))
            {
                QScopedValueRollback<bool> valueRollback(m_clearSelectionOnSceneSelectionChange, false);

                const AZ::Data::AssetId& assetId = executionItem->GetAssetId();
                
                GeneralRequestBus::Broadcast(&GeneralRequests::OpenScriptCanvasAssetId
                    , SourceHandle(nullptr, assetId.m_guid, {}), Tracker::ScriptCanvasFileState::UNMODIFIED);
            }
        }
    }

    void LoggingWindowSession::OnLogDoubleClicked(const QModelIndex& modelIndex)
    {
        ExecutionLogTreeItem* executionItem = ResolveExecutionItem(modelIndex);

        if (executionItem)
        {
            const AZ::Data::AssetId& assetId = executionItem->GetAssetId();

            bool isAssetOpen = false;
            GeneralRequestBus::BroadcastResult(isAssetOpen, &GeneralRequests::IsScriptCanvasAssetOpen, SourceHandle(nullptr, assetId.m_guid, {}));

            GeneralRequestBus::Broadcast(&GeneralRequests::OpenScriptCanvasAssetId
                , SourceHandle(nullptr, assetId.m_guid, {}), Tracker::ScriptCanvasFileState::UNMODIFIED);

            AZ::EntityId graphCanvasGraphId;
            GeneralRequestBus::BroadcastResult(graphCanvasGraphId, &GeneralRequests::FindGraphCanvasGraphIdByAssetId
                , SourceHandle(nullptr, assetId.m_guid, {}));
            
            if (isAssetOpen)
            {
                FocusOnElement(assetId, executionItem->GetScriptCanvasAssetNodeId());
            }
            else
            {
                m_assetId = assetId;
                m_assetNodeId = executionItem->GetScriptCanvasAssetNodeId();

                m_focusDelayTimer.stop();
                m_focusDelayTimer.start();
            }
        }
    }

    void LoggingWindowSession::OnLogSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
    {
        for (const QModelIndex& deselectedIndex : deselected.indexes())
        {
            if (deselectedIndex.column() != 0)
            {
                continue;
            }

            ExecutionLogTreeItem* executionItem = ResolveExecutionItem(deselectedIndex);

            if (executionItem)
            {
                RemoveHighlight(executionItem->GetAssetId(), executionItem->GetScriptCanvasAssetNodeId());
            }
        }

        for (const QModelIndex& selectedIndex : selected.indexes())
        {
            if (selectedIndex.column() != 0)
            {
                continue;
            }

            ExecutionLogTreeItem* executionItem = ResolveExecutionItem(selectedIndex);

            if (executionItem)
            {
                HighlightElement(executionItem->GetAssetId(), executionItem->GetScriptCanvasAssetNodeId());
            }
        }
    }

    ExecutionLogTreeItem* LoggingWindowSession::ResolveExecutionItem(const QModelIndex& proxyModelIndex)
    {
        QModelIndex sourceIndex = m_filterModel->mapToSource(proxyModelIndex);
        DebugLogTreeItem* treeItem = static_cast<DebugLogTreeItem*>(sourceIndex.internalPointer());

        DebugLogTreeItem* parentItem = static_cast<DebugLogTreeItem*>(treeItem->GetParent());
        ExecutionLogTreeItem* executionItem = azrtti_cast<ExecutionLogTreeItem*>(treeItem);

        while (executionItem == nullptr && parentItem != nullptr)
        {
            executionItem = azrtti_cast<ExecutionLogTreeItem*>(parentItem);
            parentItem = static_cast<DebugLogTreeItem*>(parentItem->GetParent());
        }

        return executionItem;
    }

    void LoggingWindowSession::HandleQueuedFocus()
    {
        AZ::EntityId activeGraphCanvasGraphId;
        GeneralRequestBus::BroadcastResult(activeGraphCanvasGraphId, &GeneralRequests::GetActiveGraphCanvasGraphId);

        AZ::EntityId graphCanvasGraphId;
        GeneralRequestBus::BroadcastResult(graphCanvasGraphId, &GeneralRequests::FindGraphCanvasGraphIdByAssetId, SourceHandle(nullptr, m_assetId.m_guid, {}));

        if (activeGraphCanvasGraphId == graphCanvasGraphId)
        {
            FocusOnElement(m_assetId, m_assetNodeId);
            
            m_focusDelayTimer.stop();

            m_assetId.SetInvalid();
            m_assetNodeId.SetInvalid();
        }
    }

    void LoggingWindowSession::FocusOnElement(const AZ::Data::AssetId& assetId, const AZ::EntityId& assetNodeId)
    {
        GraphCanvas::FocusConfig focusConfig;

        AZ::EntityId scriptCanvasNodeId;
        AssetGraphSceneBus::BroadcastResult(scriptCanvasNodeId, &AssetGraphScene::FindEditorNodeIdByAssetNodeId, SourceHandle(nullptr, assetId.m_guid, {}), assetNodeId);

        GraphCanvas::NodeId graphCanvasNodeId;
        SceneMemberMappingRequestBus::EventResult(graphCanvasNodeId, scriptCanvasNodeId, &SceneMemberMappingRequests::GetGraphCanvasEntityId);

        if (GraphCanvas::GraphUtils::IsNodeGroup(graphCanvasNodeId))
        {
            focusConfig.m_spacingType = GraphCanvas::FocusConfig::SpacingType::GridStep;
            focusConfig.m_spacingAmount = 1;
        }
        else
        {
            focusConfig.m_spacingType = GraphCanvas::FocusConfig::SpacingType::Scalar;
            focusConfig.m_spacingAmount = 2;
        }

        AZStd::vector< AZ::EntityId > memberIds = { graphCanvasNodeId };

        GraphCanvas::GraphUtils::FocusOnElements(memberIds, focusConfig);

        {
            QScopedValueRollback<bool> maintainSelection(m_clearSelectionOnSceneSelectionChange, false);

            RemoveHighlight(assetId, assetNodeId);

            GraphCanvas::GraphId graphId;
            GraphCanvas::SceneMemberRequestBus::EventResult(graphId, graphCanvasNodeId, &GraphCanvas::SceneMemberRequests::GetScene);

            GraphCanvas::SceneRequestBus::Event(graphId, &GraphCanvas::SceneRequests::ClearSelection);

            GraphCanvas::SceneMemberUIRequestBus::Event(graphCanvasNodeId, &GraphCanvas::SceneMemberUIRequests::SetSelected, true);
        }
    }

    void LoggingWindowSession::HighlightElement(const AZ::Data::AssetId& assetId, const AZ::EntityId& assetNodeId)
    {
        AZ::EntityId graphCanvasGraphId;
        GeneralRequestBus::BroadcastResult(graphCanvasGraphId, &GeneralRequests::FindGraphCanvasGraphIdByAssetId, SourceHandle(nullptr, assetId.m_guid, {}));

        if (graphCanvasGraphId.IsValid())
        {
            AZ::EntityId scriptCanvasNodeId;
            AssetGraphSceneBus::BroadcastResult(scriptCanvasNodeId, &AssetGraphScene::FindEditorNodeIdByAssetNodeId, SourceHandle(nullptr, assetId.m_guid, {}), assetNodeId);

            GraphCanvas::NodeId graphCanvasNodeId;
            SceneMemberMappingRequestBus::EventResult(graphCanvasNodeId, scriptCanvasNodeId, &SceneMemberMappingRequests::GetGraphCanvasEntityId);

            GraphCanvas::SceneMemberGlowOutlineConfiguration glowConfiguration;

            glowConfiguration.m_sceneMember = graphCanvasNodeId;

            glowConfiguration.m_blurRadius = 5;

            glowConfiguration.m_pen = QPen();
            glowConfiguration.m_pen.setBrush(QColor(243, 129, 29));
            glowConfiguration.m_pen.setWidth(5);

            glowConfiguration.m_pulseRate = AZStd::chrono::milliseconds(2500);
            glowConfiguration.m_zValue = 0;

            GraphCanvas::GraphicsEffectId effectId;
            GraphCanvas::SceneRequestBus::EventResult(effectId, graphCanvasGraphId, &GraphCanvas::SceneRequests::CreateGlowOnSceneMember, glowConfiguration);

            auto effectIter = m_highlightEffects.find(assetNodeId);

            if (effectIter != m_highlightEffects.end())
            {
                GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::CancelGraphicsEffect, effectIter->second);
            }

            m_highlightEffects[assetNodeId] = effectId;
        }
    }

    void LoggingWindowSession::RemoveHighlight(const AZ::Data::AssetId& assetId, const AZ::EntityId& assetNodeId)
    {
        auto effectIter = m_highlightEffects.find(assetNodeId);

        if (effectIter != m_highlightEffects.end())
        {
            AZ::EntityId graphCanvasGraphId;
            GeneralRequestBus::BroadcastResult(graphCanvasGraphId, &GeneralRequests::FindGraphCanvasGraphIdByAssetId, SourceHandle(nullptr, assetId.m_guid, {}));

            if (graphCanvasGraphId.IsValid())
            {
                GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::CancelGraphicsEffect, effectIter->second);
            }

            m_highlightEffects.erase(effectIter);
        }
    }

    void LoggingWindowSession::ScrollToSelection()
    {
        for (auto selectedIndex : m_ui->logTree->selectionModel()->selectedIndexes())
        {
            m_ui->logTree->scrollTo(selectedIndex);
        }
    }

    void LoggingWindowSession::ClearLoggingSelection()
    {
        if (m_clearSelectionOnSceneSelectionChange)
        {
            m_ui->logTree->clearSelection();
        }
    }

#include <Editor/View/Widgets/LoggingPanel/moc_LoggingWindowSession.cpp>
}
