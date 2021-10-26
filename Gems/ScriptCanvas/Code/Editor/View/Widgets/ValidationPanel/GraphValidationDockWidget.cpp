/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QButtonGroup>

#include <AzQtComponents/Components/ToastNotification.h>
#include <GraphCanvas/Components/Connections/ConnectionBus.h>
#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/Nodes/Group/NodeGroupBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/StyleBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Utils/GraphUtils.h>
#include <GraphCanvas/Utils/NodeNudgingController.h>

#include <Editor/View/Widgets/ValidationPanel/GraphValidationDockWidget.h>
#include <Editor/View/Widgets/ValidationPanel/ui_GraphValidationPanel.h>
#include <Editor/View/Widgets/ValidationPanel/GraphValidationDockWidgetBus.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>
#include <Editor/Nodes/NodeCreateUtils.h>
#include <Editor/View/Widgets/VariablePanel/VariableDockWidget.h>

#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Core/ConnectionBus.h>
#include <ScriptCanvas/Core/NodeBus.h>
#include <ScriptCanvas/Debugger/ValidationEvents/DataValidation/DataValidationEvents.h>
#include <ScriptCanvas/Debugger/ValidationEvents/ExecutionValidation/ExecutionValidationEvents.h>
#include <ScriptCanvas/Debugger/ValidationEvents/ValidationEffects/HighlightEffect.h>
#include <ScriptCanvas/Debugger/ValidationEvents/ValidationEffects/GreyOutEffect.h>
#include <ScriptCanvas/GraphCanvas/MappingBus.h>
#include <ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>
#include <ScriptCanvas/Variable/VariableBus.h>

namespace ScriptCanvasEditor
{
    /////////////////////////////////////
    // HighlightElementValidationEffect
    /////////////////////////////////////

    HighlightElementValidationEffect::HighlightElementValidationEffect()
    {
        m_templateConfiguration.m_blurRadius = 5;

        m_templateConfiguration.m_pen = QPen();
        m_templateConfiguration.m_pen.setBrush(Qt::red);
        m_templateConfiguration.m_pen.setWidth(5);

        m_templateConfiguration.m_zValue = 0;
        m_templateConfiguration.m_pulseRate = AZStd::chrono::milliseconds(2500);
    }

    HighlightElementValidationEffect::HighlightElementValidationEffect(const QColor& color)
        : HighlightElementValidationEffect()
    {
        m_templateConfiguration.m_pen.setBrush(color);
    }

    HighlightElementValidationEffect::HighlightElementValidationEffect(const GraphCanvas::SceneMemberGlowOutlineConfiguration& glowConfiguration)
        : m_templateConfiguration(glowConfiguration)
    {
    }

    void HighlightElementValidationEffect::AddTarget(const AZ::EntityId& scriptCanvasTargetId)
    {
        AZ::EntityId graphCanvasMemberId;
        SceneMemberMappingRequestBus::EventResult(graphCanvasMemberId, scriptCanvasTargetId, &SceneMemberMappingRequests::GetGraphCanvasEntityId);

        graphCanvasMemberId = GraphCanvas::GraphUtils::FindVisibleElement(graphCanvasMemberId);

        m_targets.emplace_back(graphCanvasMemberId);
    }

    void HighlightElementValidationEffect::DisplayEffect(const GraphCanvas::GraphId& graphId)
    {
        for (const auto& targetId : m_targets)
        {
            GraphCanvas::SceneMemberGlowOutlineConfiguration glowConfiguration = m_templateConfiguration;

            glowConfiguration.m_sceneMember = targetId;
            GraphCanvas::SceneMemberUIRequestBus::EventResult(glowConfiguration.m_zValue, targetId, &GraphCanvas::SceneMemberUIRequests::GetZValue);

            GraphCanvas::GraphicsEffectId effectId;
            GraphCanvas::SceneRequestBus::EventResult(effectId, graphId, &GraphCanvas::SceneRequests::CreateGlowOnSceneMember, glowConfiguration);

            if (effectId.IsValid())
            {
                m_graphicEffectIds.emplace_back(effectId);
            }
        }

        m_graphId = graphId;
    }

    void HighlightElementValidationEffect::CancelEffect()
    {
        for (const auto& graphicsEffectId : m_graphicEffectIds)
        {
            GraphCanvas::SceneRequestBus::Event(m_graphId, &GraphCanvas::SceneRequests::CancelGraphicsEffect, graphicsEffectId);
        }

        m_graphicEffectIds.clear();
    }

    ///////////////////////////////
    // UnusedNodeValidationEffect
    ///////////////////////////////

    constexpr const char* UnusedSelector = ":unused";
    constexpr const char* UnknownUseState = ":partially_unused";

    void UnusedNodeValidationEffect::AddUnusedNode(const AZ::EntityId& scriptCanvasNodeId)
    {
        AZ::EntityId graphCanvasMemberId;
        SceneMemberMappingRequestBus::EventResult(graphCanvasMemberId, scriptCanvasNodeId, &SceneMemberMappingRequests::GetGraphCanvasEntityId);

        auto insertResult = m_rootUnusedNodes.insert(graphCanvasMemberId);

        if (!insertResult.second)
        {
            return;
        }

        m_isDirty = true;

        m_unprocessedIds.insert(graphCanvasMemberId);
    }

    void UnusedNodeValidationEffect::RemoveUnusedNode(const AZ::EntityId& scriptCanvasNodeId)
    {
        AZ::EntityId graphCanvasMemberId;
        SceneMemberMappingRequestBus::EventResult(graphCanvasMemberId, scriptCanvasNodeId, &SceneMemberMappingRequests::GetGraphCanvasEntityId);

        size_t removeCount = m_rootUnusedNodes.erase(graphCanvasMemberId);

        if (removeCount == 0)
        {
            return;
        }

        m_isDirty = true;

        ClearStyleSelectors();

        m_unprocessedIds = m_rootUnusedNodes;
        m_inactiveNodes.clear();
    }

    void UnusedNodeValidationEffect::DisplayEffect(const GraphCanvas::GraphId&)
    {
        if (!m_isDirty)
        {
            return;
        }

        AZStd::unordered_set< GraphCanvas::NodeId > processedIds;

        m_isDirty = false;

        while (!m_unprocessedIds.empty())
        {
            AZ::EntityId currentMemberId = (*m_unprocessedIds.begin());
            m_unprocessedIds.erase(m_unprocessedIds.begin());

            processedIds.insert(currentMemberId);

            AZStd::vector< GraphCanvas::SlotId > slotIds;
            GraphCanvas::NodeRequestBus::EventResult(slotIds, currentMemberId, &GraphCanvas::NodeRequests::GetSlotIds);
            
            bool isFullyDisabled = true;

            AZStd::unordered_set< GraphCanvas::ConnectionId > connectionsToStylize;

            for (const auto& slotId : slotIds)
            {
                AZStd::vector< GraphCanvas::ConnectionId > connectionIds;
                GraphCanvas::SlotRequestBus::EventResult(connectionIds, slotId, &GraphCanvas::SlotRequests::GetConnections);

                GraphCanvas::ConnectionType connectionType = GraphCanvas::ConnectionType::CT_Invalid;
                GraphCanvas::SlotRequestBus::EventResult(connectionType, slotId, &GraphCanvas::SlotRequests::GetConnectionType);

                GraphCanvas::SlotType slotType = GraphCanvas::SlotTypes::DataSlot;
                GraphCanvas::SlotRequestBus::EventResult(slotType, slotId, &GraphCanvas::SlotRequests::GetSlotType);

                for (const auto& connectionId : connectionIds)
                {
                    if (slotType == GraphCanvas::SlotTypes::DataSlot
                        || connectionType == GraphCanvas::ConnectionType::CT_Output)
                    {
                        connectionsToStylize.insert(connectionId);
                    }

                    if (slotType == GraphCanvas::SlotTypes::ExecutionSlot)
                    {
                        if (connectionType == GraphCanvas::ConnectionType::CT_Output)
                        {
                            GraphCanvas::Endpoint targetEndpoint;
                            GraphCanvas::ConnectionRequestBus::EventResult(targetEndpoint, connectionId, &GraphCanvas::ConnectionRequests::GetTargetEndpoint);

                            if (processedIds.find(targetEndpoint.GetNodeId()) == processedIds.end())
                            {
                                m_unprocessedIds.insert(targetEndpoint.GetNodeId());
                            }
                        }
                        else if (connectionType == GraphCanvas::ConnectionType::CT_Input)
                        {
                            GraphCanvas::Endpoint sourceEndpoint;
                            GraphCanvas::ConnectionRequestBus::EventResult(sourceEndpoint, connectionId, &GraphCanvas::ConnectionRequests::GetSourceEndpoint);

                            // If we find a node that we are unsure about its activation state.
                            // Don't mark ourselves as fully disabled.
                            if (m_inactiveNodes.find(sourceEndpoint.GetNodeId()) == m_inactiveNodes.end())
                            {
                                isFullyDisabled = false;
                            }
                        }
                    }
                }
            }

            const char* selectorState;

            if (isFullyDisabled)
            {
                selectorState = UnusedSelector;
                m_inactiveNodes.insert(currentMemberId);
            }
            else
            {
                selectorState = UnknownUseState;
            }

            ApplySelector(currentMemberId, selectorState);

            for (const auto& connectionId : connectionsToStylize)
            {
                ApplySelector(connectionId, selectorState);
            }
        }
    }

    void UnusedNodeValidationEffect::CancelEffect()
    {
        // The remove node logic handles these updates
        return;
    }

    void UnusedNodeValidationEffect::ClearStyleSelectors()
    {
        while (!m_styleSelectors.empty())
        {
            auto mapIter = m_styleSelectors.begin();
            RemoveSelector(mapIter->first);
        }
    }

    void UnusedNodeValidationEffect::ApplySelector(const AZ::EntityId& memberId, AZStd::string_view styleSelector)
    {
        RemoveSelector(memberId);

        GraphCanvas::StyledEntityRequestBus::Event(memberId, &GraphCanvas::StyledEntityRequests::AddSelectorState, styleSelector.data());
        m_styleSelectors[memberId] = styleSelector;
    }

    void UnusedNodeValidationEffect::RemoveSelector(const AZ::EntityId& memberId)
    {
        auto mapIter = m_styleSelectors.find(memberId);

        if (mapIter != m_styleSelectors.end())
        {
            GraphCanvas::StyledEntityRequestBus::Event(memberId, &GraphCanvas::StyledEntityRequests::RemoveSelectorState, mapIter->second.data());
            m_styleSelectors.erase(mapIter);
        }
    }

    /////////////////////////
    // GraphValidationModel
    /////////////////////////
    
    GraphValidationModel::GraphValidationModel()
        : m_errorIcon(":/ScriptCanvasEditorResources/Resources/error_icon.png")
        , m_warningIcon(":/ScriptCanvasEditorResources/Resources/warning_symbol.png")
        , m_messageIcon(":/ScriptCanvasEditorResources/Resources/message_icon.png")
        , m_autoFixIcon(":/ScriptCanvasEditorResources/Resources/wrench_icon.png")
    {
    }
    
    GraphValidationModel::~GraphValidationModel()
    {
        m_validationResults.ClearResults();
    }

    void GraphValidationModel::RunValidation(const ScriptCanvas::ScriptCanvasId& scriptCanvasId)
    {
        layoutAboutToBeChanged();

        if (scriptCanvasId.IsValid())
        {
            AZ::EBusAggregateResults<AZStd::pair<ScriptCanvas::ScriptCanvasId, ScriptCanvas::ValidationResults>> results;
            ScriptCanvas::ValidationRequestBus::EventResult(results, scriptCanvasId, &ScriptCanvas::ValidationRequests::GetValidationResults);
            
            for (auto r : results.values)
            {
                if (r.first == scriptCanvasId)
                {
                    for (auto e : r.second.GetEvents())
                    {
                        m_validationResults.AddValidationEvent(e.get());
                    }
                }
            }
        }
        
        layoutChanged();

        GraphValidatorDockWidgetNotificationBus::Event(ScriptCanvasEditor::AssetEditorId, &GraphValidatorDockWidgetNotifications::OnResultsChanged, m_validationResults.ErrorCount(), m_validationResults.WarningCount());
    }
    
    void GraphValidationModel::AddEvents(ScriptCanvas::ValidationResults& validationEvents)
    {
        if (validationEvents.HasErrors() || validationEvents.HasWarnings())
        {
            layoutAboutToBeChanged();
            for (auto event : validationEvents.GetEvents())
            {
                m_validationResults.AddValidationEvent(event.get());
            }
            layoutChanged();
        }

        GraphValidatorDockWidgetNotificationBus::Event(ScriptCanvasEditor::AssetEditorId, &GraphValidatorDockWidgetNotifications::OnResultsChanged, m_validationResults.ErrorCount(), m_validationResults.WarningCount());

    }

    void GraphValidationModel::Clear()
    {
        m_validationResults.ClearResults();
    }

    QModelIndex GraphValidationModel::index(int row, int column, const QModelIndex&) const
    {
        if (row < 0 || row >= m_validationResults.GetEvents().size())
        {
            return QModelIndex();
        }
        
        return createIndex(row, column, const_cast<ScriptCanvas::ValidationEvent*>(FindItemForRow(row)));
    }
    
    QModelIndex GraphValidationModel::parent(const QModelIndex&) const
    {
        return QModelIndex();
    }
    
    int GraphValidationModel::columnCount(const QModelIndex& ) const
    {
        return ColumnIndex::Count;
    }
    
    int GraphValidationModel::rowCount(const QModelIndex&) const
    {
        return static_cast<int>(m_validationResults.GetEvents().size());
    }

    QVariant GraphValidationModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation == Qt::Orientation::Vertical)
        {
            return QVariant();
        }

        if (role == Qt::DisplayRole)
        {
            switch (section)
            {
                case ColumnIndex::Description:
                {
                    return QString("Description");
                }
                break;
                default:
                    break;
            }
        }

        return QVariant();
    }
    
    QVariant GraphValidationModel::data(const QModelIndex& index, int role) const
    {
        const ScriptCanvas::ValidationEvent* validationEvent = FindItemForIndex(index);
        
        if (role == Qt::DisplayRole)
        {
            switch (index.column())
            {
                case ColumnIndex::Description:
                {
                    return QString(validationEvent->GetDescription().data());
                }
                break;
                default:
                    break;
            }
        }
        else if (role == Qt::DecorationRole)
        {
            switch (index.column())
            {
                // We always want the icon on the leftmost column. So doing away with my usual
                // Labelling to keep the spirit of what I'm after(simple table re-ordering).
                case 0:
                {
                    switch (validationEvent->GetSeverity())
                    {
                    case ScriptCanvas::ValidationSeverity::Error:
                    {
                        return m_errorIcon;
                    }
                    break;
                    case ScriptCanvas::ValidationSeverity::Warning:
                    {
                        return m_warningIcon;
                    }
                    break;
                    case ScriptCanvas::ValidationSeverity::Informative:
                    {
                        return m_messageIcon;
                    }
                    break;
                    default:
                        break;
                    }
                }
                break;
                case ColumnIndex::AutoFix:
                {
                    if (validationEvent->CanAutoFix())
                    {
                        return m_autoFixIcon;
                    }
                }
                break;
                default:
                    break;
            }
        }
        else if (role == Qt::ToolTipRole)
        {
            switch (index.column())
            {
            case ColumnIndex::Description:
            {
                return QString("%1 - %2").arg(validationEvent->GetIdentifier().c_str()).arg(validationEvent->GetTooltip().data());
            }
            case ColumnIndex::AutoFix:
            {
                if (validationEvent->CanAutoFix())
                {
                    return "A potential automatic fix can be applied for this issue. Press this button to fix the error.";
                }
            }
            }
        }
        
        return QVariant();
    }
    
    const ScriptCanvas::ValidationEvent* GraphValidationModel::FindItemForIndex(const QModelIndex& index) const
    {
        if (index.isValid())
        {
            return FindItemForRow(index.row());
        }
        
        return nullptr;
    }
    
    const ScriptCanvas::ValidationEvent* GraphValidationModel::FindItemForRow(int row) const
    {
        const auto& validationEvents = m_validationResults.GetEvents();

        if (row < 0 || row >= validationEvents.size())
        {
            return nullptr;
        }
        
        return validationEvents[row].get();
    }

    const ScriptCanvas::ValidationResults& GraphValidationModel::GetValidationResults() const
    {
        return m_validationResults;
    }
    
    ////////////////////////////////////////
    // GraphValidationSortFilterProxyModel
    ////////////////////////////////////////
    
    GraphValidationSortFilterProxyModel::GraphValidationSortFilterProxyModel()
        : m_severityFilter(ScriptCanvas::ValidationSeverity::Unknown)
    {
        // TODO: Populate the errors from the user settings
    }
    
    bool GraphValidationSortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
    {
        QAbstractItemModel* model = sourceModel();
        QModelIndex index = model->index(sourceRow, 0, sourceParent);

        const ScriptCanvas::ValidationEvent* currentItem = static_cast<const ScriptCanvas::ValidationEvent*>(index.internalPointer());

        // If our filter is set to all, we can just show the message
        bool showRow = ((m_severityFilter == ScriptCanvas::ValidationSeverity::Unknown) || (currentItem->GetSeverity() == m_severityFilter));

        if (showRow && !m_filter.isEmpty())
        {
            AZStd::string_view descriptionView = currentItem->GetDescription();

            QString description = QString::fromUtf8(descriptionView.data(), static_cast<int>(descriptionView.size()));

            if (description.lastIndexOf(m_regex) < 0)
            {
                QString errorId = QString(currentItem->GetIdentifier().c_str());

                if (errorId.lastIndexOf(m_regex) < 0)
                {
                    showRow = false;
                }
            }
        }
        
        return showRow;
    }

    void GraphValidationSortFilterProxyModel::SetFilter(const QString& filterString)
    {
        QString escapedString = QRegExp::escape(filterString);

        if (m_filter != escapedString)
        {
            m_filter = escapedString;
            m_regex = QRegExp(m_filter, Qt::CaseInsensitive);

            invalidateFilter();
        }
    }

    void GraphValidationSortFilterProxyModel::SetSeverityFilter(ScriptCanvas::ValidationSeverity severityFilter)
    {
        if (m_severityFilter != severityFilter)
        {
            m_severityFilter = severityFilter;

            invalidateFilter();
        }
    }

    ScriptCanvas::ValidationSeverity GraphValidationSortFilterProxyModel::GetSeverityFilter() const
    {
        return m_severityFilter;
    }

    bool GraphValidationSortFilterProxyModel::IsShowingErrors()
    {
        return m_severityFilter == ScriptCanvas::ValidationSeverity::Unknown || m_severityFilter == ScriptCanvas::ValidationSeverity::Error;
    }

    bool GraphValidationSortFilterProxyModel::IsShowingWarnings()
    {
        return m_severityFilter == ScriptCanvas::ValidationSeverity::Unknown || m_severityFilter == ScriptCanvas::ValidationSeverity::Warning;
    }
    
    //////////////////////////////
    // GraphValidationDockWidget
    //////////////////////////////
    
    GraphValidationDockWidget::GraphValidationDockWidget(QWidget* parent /*= nullptr*/)
        : AzQtComponents::StyledDockWidget(parent)
        , ui(new Ui::GraphValidationPanel())
        , m_proxyModel(aznew GraphValidationSortFilterProxyModel())
    {
        ui->setupUi(this);

        m_proxyModel->setSourceModel(aznew GraphValidationModel()); 
        ui->statusTableView->setModel(m_proxyModel);

        ui->statusTableView->horizontalHeader()->setStretchLastSection(false);

        ui->statusTableView->horizontalHeader()->setSectionResizeMode(GraphValidationModel::Description, QHeaderView::ResizeMode::Stretch);
        
        ui->statusTableView->horizontalHeader()->setSectionResizeMode(GraphValidationModel::AutoFix, QHeaderView::ResizeMode::Fixed);
        ui->statusTableView->horizontalHeader()->resizeSection(GraphValidationModel::AutoFix, 32);

        ui->searchWidget->SetFilterInputInterval(AZStd::chrono::milliseconds(250));

        QButtonGroup* buttonGroup = new QButtonGroup(this);
        buttonGroup->setExclusive(true);

        buttonGroup->addButton(ui->allFilter);
        buttonGroup->addButton(ui->errorOnlyFilter);
        buttonGroup->addButton(ui->warningOnlyFilter);

        ui->allFilter->setChecked(true);

        QObject::connect(ui->allFilter, &QPushButton::clicked, this, &GraphValidationDockWidget::OnSeverityFilterChanged);
        QObject::connect(ui->errorOnlyFilter, &QPushButton::clicked, this, &GraphValidationDockWidget::OnSeverityFilterChanged);
        QObject::connect(ui->warningOnlyFilter, &QPushButton::clicked, this, &GraphValidationDockWidget::OnSeverityFilterChanged);

        QObject::connect(ui->runValidation, &QToolButton::clicked, this, &GraphValidationDockWidget::OnRunValidator);
        QObject::connect(ui->fixSelected, &QPushButton::clicked, this, &GraphValidationDockWidget::FixSelected);

        QObject::connect(ui->statusTableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &GraphValidationDockWidget::OnTableSelectionChanged);

        QObject::connect(ui->statusTableView, &QTableView::doubleClicked, this, &GraphValidationDockWidget::FocusOnEvent);
        QObject::connect(ui->statusTableView, &QTableView::clicked, this, &GraphValidationDockWidget::TryAutoFixEvent);

        QObject::connect(ui->searchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged, this, &GraphValidationDockWidget::OnFilterChanged);
        
        GraphCanvas::AssetEditorNotificationBus::Handler::BusConnect(ScriptCanvasEditor::AssetEditorId);

        ui->runValidation->setEnabled(false);
        ui->fixSelected->setEnabled(false);
        ui->fixSelected->setVisible(false);

        UpdateText();
        UpdateSelectedText();

    }

    GraphValidationDockWidget::~GraphValidationDockWidget()
    {
        GraphCanvas::AssetEditorNotificationBus::Handler::BusDisconnect();
    }
    
    void GraphValidationDockWidget::OnActiveGraphChanged(const GraphCanvas::GraphId& graphCanvasGraphId)
    {
        if (graphCanvasGraphId == m_activeGraphIds.graphCanvasId)
        {
            // No change
            return;
        }

        OnToastDismissed();

        if (graphCanvasGraphId.IsValid())
        {
            ScriptCanvas::ScriptCanvasId scriptCanvasId;
            GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphCanvasGraphId);

            if (m_models.find(graphCanvasGraphId) == m_models.end())
            {
                // We have not created a model for this graph yet.
                m_models[graphCanvasGraphId] = AZStd::make_pair(scriptCanvasId, AZStd::unique_ptr<ValidationData>(aznew ValidationData(graphCanvasGraphId, scriptCanvasId)));
            }

            m_activeGraphIds.graphCanvasId = graphCanvasGraphId;
            m_activeGraphIds.scriptCanvasId = scriptCanvasId;
        }
        else
        {
            return;
        }

        if (ValidationData* valdata = m_models[m_activeGraphIds.graphCanvasId].second.get())
        {
            m_proxyModel->setSourceModel(valdata->GetModel());

            Refresh();

            GraphCanvas::SceneNotificationBus::Handler::BusDisconnect();
            GraphCanvas::SceneNotificationBus::Handler::BusConnect(m_activeGraphIds.graphCanvasId);

            ui->statusTableView->clearSelection();
        }
    }

    void GraphValidationDockWidget::Refresh()
    {
        ui->statusTableView->clearSelection();
        UpdateText();

        
        ui->runValidation->setEnabled(GetActiveData().first.IsValid());
    }

    const ScriptCanvasEditor::GraphValidationModel* GraphValidationDockWidget::GetActiveModel() const
    {
        if (m_models.contains(m_activeGraphIds.graphCanvasId))
        {
            const auto it = m_models.find(m_activeGraphIds.graphCanvasId);
            const GraphModelPair& pair = const_cast<const GraphModelPair&>(it->second);
            return pair.second->GetModel();
        }

        return nullptr;
    }

    ScriptCanvasEditor::GraphValidationDockWidget::GraphModelPair& GraphValidationDockWidget::GetActiveData()
    {
        auto iter = m_models.find(m_activeGraphIds.graphCanvasId);
        if (iter != m_models.end())
        {
            return iter->second;
        }

        static GraphModelPair invalidPair = AZStd::make_pair(AZ::EntityId(), nullptr);
        return invalidPair;
    }

    void GraphValidationDockWidget::OnSelectionChanged()
    {
        ui->statusTableView->clearSelection();
    }

    void GraphValidationDockWidget::OnConnectionDragBegin()
    {
        ui->statusTableView->clearSelection();
    }

    bool GraphValidationDockWidget::HasValidationIssues() const
    {
        const auto model = GetActiveModel();
        return model ? model->rowCount() > 0 : false;
    }
    
    void GraphValidationDockWidget::OnRunValidator(bool displayAsNotification)
    {
        ui->statusTableView->clearSelection();

        if (auto model = GetActiveData().second ? GetActiveData().second->GetModel() : nullptr)
        {
            model->Clear();
            model->RunValidation(m_activeGraphIds.scriptCanvasId);
        }
        ui->allFilter->click();

        UpdateText();

        if (!displayAsNotification)
        {
            ui->statusTableView->selectAll();
        }
        else if (HasValidationIssues())
        {
            GetActiveData().second->DisplayToast();
        }
    }
    
    void GraphValidationDockWidget::OnShowErrors()
    {
        ui->errorOnlyFilter->setChecked(true);
        OnSeverityFilterChanged();
    }
    
    void GraphValidationDockWidget::OnShowWarnings()
    {
        ui->warningOnlyFilter->setChecked(true);
        OnSeverityFilterChanged();
    }
    
    void GraphValidationDockWidget::OnTableSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
    {
        // Handle the deselection cases correctly
        for (auto modelIndex : deselected.indexes())
        {
            OnRowDeselected(m_proxyModel->mapToSource(modelIndex).row());
        }

        ui->fixSelected->setEnabled(false);

        // We want everything to be in sync visually.
        // So fake reselect everything to restart everything
        if (!selected.empty())
        {
            for (auto modelIndex : ui->statusTableView->selectionModel()->selectedIndexes())
            {
                if (modelIndex.column() == 0)
                {
                    QModelIndex sourceIndex = m_proxyModel->mapToSource(modelIndex);

                    if (auto model = GetActiveModel())
                    {
                        const ScriptCanvas::ValidationEvent* validationEvent = model->FindItemForIndex(sourceIndex);

                        if (validationEvent && validationEvent->CanAutoFix())
                        {
                            ui->fixSelected->setEnabled(true);
                        }

                        OnRowSelected(sourceIndex.row());
                    }
                }
            }

            m_unusedNodeValidationEffect.DisplayEffect(m_activeGraphIds.graphCanvasId);
        }
    }

    void GraphValidationDockWidget::FocusOnEvent(const QModelIndex& modelIndex)
    {
        if (auto model = GetActiveModel())
        {
            const ScriptCanvas::ValidationEvent* validationEvent = model->FindItemForIndex(m_proxyModel->mapToSource(modelIndex));
        
            AZ::EntityId graphCanvasMemberId;

            if (const ScriptCanvas::FocusOnEntityEffect* focusOnEntityEffect = azrtti_cast<const ScriptCanvas::FocusOnEntityEffect*>(validationEvent))
            {
                const AZ::EntityId& scriptCanvasId = focusOnEntityEffect->GetFocusTarget();

                SceneMemberMappingRequestBus::EventResult(graphCanvasMemberId, scriptCanvasId, &SceneMemberMappingRequests::GetGraphCanvasEntityId);
            }

            if (graphCanvasMemberId.IsValid())
            {
                GraphCanvas::FocusConfig focusConfig;

                if (GraphCanvas::GraphUtils::IsNodeGroup(graphCanvasMemberId))
                {
                    focusConfig.m_spacingType = GraphCanvas::FocusConfig::SpacingType::GridStep;
                    focusConfig.m_spacingAmount = 1;
                }
                else
                {
                    focusConfig.m_spacingType = GraphCanvas::FocusConfig::SpacingType::Scalar;
                    focusConfig.m_spacingAmount = 2;
                }

                AZStd::vector<AZ::EntityId> memberIds = { graphCanvasMemberId };

                GraphCanvas::GraphUtils::FocusOnElements(memberIds, focusConfig);
            }
        }
    }

    void GraphValidationDockWidget::TryAutoFixEvent(const QModelIndex& modelIndex)
    {
        if (modelIndex.column() != GraphValidationModel::ColumnIndex::AutoFix)
        {
            return;
        }

        if (auto model = GetActiveModel())
        {
            const ScriptCanvas::ValidationEvent* validationEvent = model->FindItemForIndex(m_proxyModel->mapToSource(modelIndex));

            if (!validationEvent->CanAutoFix())
            {
                return;
            }

            AutoFixEvent(validationEvent);

        }

        OnRunValidator();
    }

    void GraphValidationDockWidget::FixSelected()
    {
        {
            GraphCanvas::ScopedGraphUndoBlocker undoBlocker(m_activeGraphIds.graphCanvasId);

            for (auto modelIndex : ui->statusTableView->selectionModel()->selectedIndexes())
            {
                if (modelIndex.column() == 0)
                {
                    QModelIndex sourceIndex = m_proxyModel->mapToSource(modelIndex);
                    if (auto model = GetActiveModel())
                    {
                        const ScriptCanvas::ValidationEvent* validationEvent = model->FindItemForIndex(sourceIndex);

                        if (validationEvent->CanAutoFix())
                        {
                            AutoFixEvent(validationEvent);
                        }
                    }
                }
            }
        }

        GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, m_activeGraphIds.scriptCanvasId);

        OnRunValidator();
    }

    void GraphValidationDockWidget::OnSeverityFilterChanged()
    {
        if (ui->allFilter->isChecked())
        {
            // Using unknown as a proxy for all
            m_proxyModel->SetSeverityFilter(ScriptCanvas::ValidationSeverity::Unknown);
        }
        else if (ui->errorOnlyFilter->isChecked())
        {
            m_proxyModel->SetSeverityFilter(ScriptCanvas::ValidationSeverity::Error);
        }
        else if (ui->warningOnlyFilter->isChecked())
        {
            m_proxyModel->SetSeverityFilter(ScriptCanvas::ValidationSeverity::Warning);
        }

        UpdateText();
    }

    void GraphValidationDockWidget::OnFilterChanged(const QString& filterString)
    {
        m_proxyModel->SetFilter(filterString);
    }

    void GraphValidationDockWidget::AutoFixEvent(const ScriptCanvas::ValidationEvent* validationEvent)
    {
        if (validationEvent->GetIdCrc() == ScriptCanvas::DataValidationIds::ScopedDataConnectionCrc)
        {
            AutoFixScopedDataConnection(static_cast<const ScriptCanvas::ScopedDataConnectionEvent*>(validationEvent));
        }
        else if (validationEvent->GetIdCrc() == ScriptCanvas::DataValidationIds::InvalidVariableTypeCrc)
        {
            AutoFixDeleteInvalidVariables(static_cast<const ScriptCanvas::InvalidVariableTypeEvent*>(validationEvent));
        }
        else if (validationEvent->GetIdCrc() == ScriptCanvas::DataValidationIds::ScriptEventVersionMismatchCrc)
        {
            AutoFixScriptEventVersionMismatch(static_cast<const ScriptCanvas::ScriptEventVersionMismatch*>(validationEvent));
        }
        else
        {
            AZ_Error("ScriptCanvas", false, "Cannot auto fix event type %s despite it being marked at auto fixable", validationEvent->GetIdentifier().c_str());
        }
    }

    void GraphValidationDockWidget::AutoFixScriptEventVersionMismatch(const ScriptCanvas::ScriptEventVersionMismatch* scriptEventMismatchEvent)
    {
        {
            GraphCanvas::ScopedGraphUndoBlocker undoBlocker(m_activeGraphIds.graphCanvasId);

            AZ::EntityId graphCanvasId;
            SceneMemberMappingRequestBus::EventResult(graphCanvasId, scriptEventMismatchEvent->GetNodeId(), &SceneMemberMappingRequests::GetGraphCanvasEntityId);

            // Detach all connections
            GraphCanvas::GraphUtils::DetachNodeAndStitchConnections(graphCanvasId);

            // TODO #lsempe:
            // Notify the node to update to its latest version
            //EditorGraphRequestBus::Event(m_scriptCanvasGraphId, &EditorGraphRequests::UpdateScriptEventVersion, scriptEventMismatchEvent->GetNodeId());
        }

        GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, m_activeGraphIds.scriptCanvasId);

    }

    void GraphValidationDockWidget::AutoFixDeleteInvalidVariables(const ScriptCanvas::InvalidVariableTypeEvent* invalidVariableEvent)
    {
        {
            GraphCanvas::ScopedGraphUndoBlocker undoBlocker(m_activeGraphIds.graphCanvasId);

            AZStd::vector<NodeIdPair> variableNodes;
            EditorGraphRequestBus::EventResult(variableNodes, m_activeGraphIds.scriptCanvasId, &EditorGraphRequests::GetVariableNodes, invalidVariableEvent->GetVariableId());
            for (auto& variableNode : variableNodes)
            {
                GraphCanvas::GraphUtils::DetachNodeAndStitchConnections(variableNode.m_graphCanvasId);
            }

            ScriptCanvas::GraphVariableManagerRequestBus::Event(m_activeGraphIds.scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::RemoveVariable, invalidVariableEvent->GetVariableId());
        }

        GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, m_activeGraphIds.scriptCanvasId);

    }

    void GraphValidationDockWidget::AutoFixScopedDataConnection(const ScriptCanvas::ScopedDataConnectionEvent* connectionEvent)
    {
        AZStd::unordered_set< AZ::EntityId > createdNodes;

        {
            GraphCanvas::ScopedGraphUndoBlocker undoBlocker(m_activeGraphIds.graphCanvasId);

            const AZ::EntityId& scriptCanvasConnectionId = connectionEvent->GetConnectionId();

            // Information gathering step
            ScriptCanvas::Endpoint scriptCanvasSourceEndpoint;
            ScriptCanvas::ConnectionRequestBus::EventResult(scriptCanvasSourceEndpoint, scriptCanvasConnectionId, &ScriptCanvas::ConnectionRequests::GetSourceEndpoint);

            // Going to match the visual expectation here, and always have it create a new variable and store the value
            // at this point in time.
            ScriptCanvas::VariableId targetVariableId;

            ScriptCanvas::Data::Type variableType = ScriptCanvas::Data::Type::Invalid();
            ScriptCanvas::NodeRequestBus::EventResult(variableType, scriptCanvasSourceEndpoint.GetNodeId(), &ScriptCanvas::NodeRequests::GetSlotDataType, scriptCanvasSourceEndpoint.GetSlotId());

            if (!variableType.IsValid())
            {
                AZ_Error("ScriptCanvas", false, "Could not auto fix latent connection(%s) because connection did not return a valid data type.", scriptCanvasConnectionId.ToString().c_str());
                return;
            }

            const AZStd::string& varName = VariableDockWidget::FindDefaultVariableName(m_activeGraphIds.scriptCanvasId);

            ScriptCanvas::Datum datum(variableType, ScriptCanvas::Datum::eOriginality::Original);

            AZ::Outcome<ScriptCanvas::VariableId, AZStd::string> outcome = AZ::Failure(AZStd::string());
            ScriptCanvas::GraphVariableManagerRequestBus::EventResult(outcome, m_activeGraphIds.scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::AddVariable, varName, datum, false);

            if (outcome.IsSuccess())
            {
                targetVariableId = outcome.GetValue();
            }
            else
            {
                AZ_Error("ScriptCanvas", false, "Could not auto fix latent connection(%s) because variable creation failed with the message: %s", scriptCanvasConnectionId.ToString().c_str(), outcome.GetError().c_str());
                return;
            }

            // Convert elements over to GraphCanvas to begin interactions with the visual front end.
            GraphCanvas::ConnectionId graphCanvasConnectionId;
            SceneMemberMappingRequestBus::EventResult(graphCanvasConnectionId, scriptCanvasConnectionId, &SceneMemberMappingRequests::GetGraphCanvasEntityId);

            GraphCanvas::Endpoint sourceEndpoint;
            GraphCanvas::ConnectionRequestBus::EventResult(sourceEndpoint, graphCanvasConnectionId, &GraphCanvas::ConnectionRequests::GetSourceEndpoint);

            GraphCanvas::Endpoint targetEndpoint;
            GraphCanvas::ConnectionRequestBus::EventResult(targetEndpoint, graphCanvasConnectionId, &GraphCanvas::ConnectionRequests::GetTargetEndpoint);

            AZ::EntityId gridId;
            GraphCanvas::SceneRequestBus::EventResult(gridId, m_activeGraphIds.graphCanvasId, &GraphCanvas::SceneRequests::GetGrid);

            AZ::Vector2 gridStep(0,0);
            GraphCanvas::GridRequestBus::EventResult(gridStep, gridId, &GraphCanvas::GridRequests::GetMinorPitch);

            AZStd::unordered_set< AZ::EntityId > deletedMemberIds;
            deletedMemberIds.insert(graphCanvasConnectionId);
            
            // Inserting the Set into the execution flow
            {
                // Map of all of the execution outs on the latent node to Endpoints.
                AZStd::unordered_multimap< GraphCanvas::Endpoint, GraphCanvas::ConnectionId > sourceExecutionMapping;

                AZStd::vector< GraphCanvas::SlotId > slotIds;
                GraphCanvas::NodeRequestBus::EventResult(slotIds, sourceEndpoint.GetNodeId(), &GraphCanvas::NodeRequests::FindVisibleSlotIdsByType, GraphCanvas::ConnectionType::CT_Output, GraphCanvas::SlotTypes::ExecutionSlot);

                for (const GraphCanvas::SlotId& slotId : slotIds)
                {
                    AZStd::vector< GraphCanvas::ConnectionId > connectionIds;
                    GraphCanvas::SlotRequestBus::EventResult(connectionIds, slotId, &GraphCanvas::SlotRequests::GetConnections);

                    GraphCanvas::Endpoint executionSource(sourceEndpoint.GetNodeId(), slotId);

                    for (const GraphCanvas::ConnectionId& connectionId : connectionIds)
                    {
                        sourceExecutionMapping.insert(AZStd::make_pair(executionSource, connectionId));
                    }
                }

                if (!sourceExecutionMapping.empty())
                {
                    AZ::Vector2 position;
                    GraphCanvas::Endpoint lastEndpoint;
                    AZ::EntityId setVariableGraphCanvasId;

                    AZStd::vector< GraphCanvas::Endpoint> dataEndpoints;
                    dataEndpoints.push_back(sourceEndpoint);

                    GraphCanvas::CreateConnectionsBetweenConfig connectionConfig;
                    connectionConfig.m_connectionType = GraphCanvas::CreateConnectionsBetweenConfig::CreationType::SingleConnection;

                    for (const auto& sourcePair : sourceExecutionMapping)
                    {
                        const GraphCanvas::Endpoint& executionSourceEndpoint = sourcePair.first;
                        const GraphCanvas::ConnectionId& executionTargetConnectionId = sourcePair.second;

                        if (lastEndpoint != executionSourceEndpoint)
                        {
                            if (!lastEndpoint.IsValid())
                            {
                                QGraphicsItem* sourceItem;
                                GraphCanvas::SceneMemberUIRequestBus::EventResult(sourceItem, sourceEndpoint.GetNodeId(), &GraphCanvas::SceneMemberUIRequests::GetRootGraphicsItem);

                                if (sourceItem)
                                {
                                    QRectF sourceBoundingRect = sourceItem->sceneBoundingRect();

                                    position.SetX(aznumeric_cast<float>(sourceBoundingRect.right() + gridStep.GetX()));
                                    position.SetY(aznumeric_cast<float>(sourceBoundingRect.top()));
                                }
                            }

                            NodeIdPair createdNodePair = Nodes::CreateSetVariableNode(targetVariableId, m_activeGraphIds.scriptCanvasId);

                            setVariableGraphCanvasId = createdNodePair.m_graphCanvasId;
                            GraphCanvas::SceneRequestBus::Event(m_activeGraphIds.graphCanvasId, &GraphCanvas::SceneRequests::AddNode, setVariableGraphCanvasId, position, false);

                            createdNodes.insert(setVariableGraphCanvasId);

                            position += gridStep;

                            connectionConfig.m_createdConnections.clear();
                            GraphCanvas::GraphUtils::CreateConnectionsBetween(dataEndpoints, setVariableGraphCanvasId, connectionConfig);

                            lastEndpoint = executionSourceEndpoint;
                        }

                        GraphCanvas::ConnectionSpliceConfig spliceConfig;
                        spliceConfig.m_allowOpportunisticConnections = false;

                        GraphCanvas::GraphUtils::SpliceNodeOntoConnection(setVariableGraphCanvasId, executionTargetConnectionId, spliceConfig);
                    }
                }
                else
                {
                    NodeIdPair setVariableNodeIdPair = Nodes::CreateSetVariableNode(targetVariableId, m_activeGraphIds.scriptCanvasId);

                    createdNodes.insert(setVariableNodeIdPair.m_graphCanvasId);

                    QRectF sourceBoundingRect;
                    QGraphicsItem* graphicsItem = nullptr;
                    GraphCanvas::SceneMemberUIRequestBus::EventResult(graphicsItem, sourceEndpoint.GetNodeId(), &GraphCanvas::SceneMemberUIRequests::GetRootGraphicsItem);

                    if (graphicsItem)
                    {
                        sourceBoundingRect = graphicsItem->sceneBoundingRect();
                    }

                    AZ::Vector2 position = AZ::Vector2(aznumeric_cast<float>(sourceBoundingRect.right() + gridStep.GetX()), aznumeric_cast<float>(sourceBoundingRect.top()));
                    GraphCanvas::SceneRequestBus::Event(m_activeGraphIds.graphCanvasId, &GraphCanvas::SceneRequests::AddNode, setVariableNodeIdPair.m_graphCanvasId, position, false);

                    AZStd::vector< GraphCanvas::Endpoint> endpoints;
                    endpoints.reserve(slotIds.size() + 1);
                    endpoints.emplace_back(sourceEndpoint);

                    for (const GraphCanvas::SlotId& slotId : slotIds)
                    {
                        endpoints.emplace_back(sourceEndpoint.GetNodeId(), slotId);
                    }

                    GraphCanvas::CreateConnectionsBetweenConfig connectionConfig;
                    connectionConfig.m_connectionType = GraphCanvas::CreateConnectionsBetweenConfig::CreationType::FullyConnected;

                    GraphCanvas::GraphUtils::CreateConnectionsBetween(endpoints, setVariableNodeIdPair.m_graphCanvasId, connectionConfig);
                }
            }

            // Inserting the get into the execution flow
            {
                NodeIdPair getVariableNodeIdPair = Nodes::CreateGetVariableNode(targetVariableId, m_activeGraphIds.scriptCanvasId);

                createdNodes.insert(getVariableNodeIdPair.m_graphCanvasId);

                QRectF targetBoundingRect;
                QGraphicsItem* graphicsItem = nullptr;
                GraphCanvas::SceneMemberUIRequestBus::EventResult(graphicsItem, targetEndpoint.GetNodeId(), &GraphCanvas::SceneMemberUIRequests::GetRootGraphicsItem);

                if (graphicsItem)
                {
                    targetBoundingRect = graphicsItem->sceneBoundingRect();
                }

                AZ::Vector2 position = AZ::Vector2(aznumeric_cast<float>(targetBoundingRect.left() - gridStep.GetX()), aznumeric_cast<float>(targetBoundingRect.top()));

                QGraphicsItem* newGraphicsItem = nullptr;
                GraphCanvas::SceneMemberUIRequestBus::EventResult(newGraphicsItem, getVariableNodeIdPair.m_graphCanvasId, &GraphCanvas::SceneMemberUIRequests::GetRootGraphicsItem);

                if (newGraphicsItem)
                {
                    position.SetX(aznumeric_cast<float>(position.GetX() - newGraphicsItem->sceneBoundingRect().width()));
                }

                GraphCanvas::SceneRequestBus::Event(m_activeGraphIds.graphCanvasId, &GraphCanvas::SceneRequests::AddNode, getVariableNodeIdPair.m_graphCanvasId, position, false);

                AZStd::vector< GraphCanvas::SlotId > slotIds;
                GraphCanvas::NodeRequestBus::EventResult(slotIds, targetEndpoint.GetNodeId(), &GraphCanvas::NodeRequests::FindVisibleSlotIdsByType, GraphCanvas::ConnectionType::CT_Input, GraphCanvas::SlotTypes::ExecutionSlot);

                AZStd::vector< GraphCanvas::Endpoint > executionSourceEndpoints;
                AZStd::vector< GraphCanvas::Endpoint > validTargetEndpoints;
                validTargetEndpoints.push_back(targetEndpoint);

                for (const GraphCanvas::SlotId& slotId : slotIds)
                {
                    AZStd::vector< GraphCanvas::ConnectionId > connectionIds;
                    GraphCanvas::SlotRequestBus::EventResult(connectionIds, slotId, &GraphCanvas::SlotRequests::GetConnections);

                    validTargetEndpoints.emplace_back(targetEndpoint.GetNodeId(), slotId);

                    for (const GraphCanvas::ConnectionId& connectionId : connectionIds)
                    {
                        GraphCanvas::Endpoint targetExecutionSourceEndpoint;
                        GraphCanvas::ConnectionRequestBus::EventResult(targetExecutionSourceEndpoint, connectionId, &GraphCanvas::ConnectionRequests::GetSourceEndpoint);

                        executionSourceEndpoints.push_back(targetExecutionSourceEndpoint);

                        deletedMemberIds.insert(connectionId);
                    }
                }

                // Hook up all of the connection inputs
                if (!executionSourceEndpoints.empty())
                {
                    GraphCanvas::CreateConnectionsBetweenConfig config;
                    config.m_connectionType = GraphCanvas::CreateConnectionsBetweenConfig::CreationType::FullyConnected;

                    GraphCanvas::GraphUtils::CreateConnectionsBetween(executionSourceEndpoints, getVariableNodeIdPair.m_graphCanvasId, config);
                }

                // Hook up to the actual target endpoints
                GraphCanvas::CreateConnectionsBetweenConfig config;
                config.m_connectionType = GraphCanvas::CreateConnectionsBetweenConfig::CreationType::SinglePass;

                GraphCanvas::GraphUtils::CreateConnectionsBetween(validTargetEndpoints, getVariableNodeIdPair.m_graphCanvasId, config);
            }

            GraphCanvas::SceneRequestBus::Event(m_activeGraphIds.graphCanvasId, &GraphCanvas::SceneRequests::Delete, deletedMemberIds);
        }

        GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, m_activeGraphIds.scriptCanvasId);

        GraphCanvas::NodeNudgingController nudgingController;
        nudgingController.SetGraphId(m_activeGraphIds.graphCanvasId);

        nudgingController.StartNudging(createdNodes);
        nudgingController.FinalizeNudging();
    }

    void GraphValidationDockWidget::UpdateText()
    {
        int errorCount = 0;
        int warningCount = 0;

        auto& tabdata = GetActiveData();
        if (tabdata.first.IsValid())
        {
            if (auto model = GetActiveModel())
            {
                // Clear our the text Filter
                ui->searchWidget->SetTextFilter("");
                m_proxyModel->SetFilter("");

                errorCount = model->GetValidationResults().ErrorCount();
                warningCount = model->GetValidationResults().WarningCount();
            }
        }

        ui->errorOnlyFilter->setText(QString("%1 Errors").arg(errorCount));
        ui->warningOnlyFilter->setText(QString("%1 Warnings").arg(warningCount));

    }
    void GraphValidationDockWidget::OnRowSelected(int row)
    {
        // If we already have an effect on this row, restart it to maintain visual consistency of the glows.
        if (auto effect = GetActiveData().second->GetEffect(row))
        {
            effect->CancelEffect();
            effect->DisplayEffect(m_activeGraphIds.graphCanvasId);
            return;
        }

        if (auto model = GetActiveModel())
        {
            const ScriptCanvas::ValidationEvent* validationEvent = model->FindItemForRow(row);

            if (const ScriptCanvas::HighlightEntityEffect* highlightEntity = azrtti_cast<const ScriptCanvas::HighlightEntityEffect*>(validationEvent))
            {
                HighlightElementValidationEffect* highlightEffect = aznew HighlightElementValidationEffect();

                highlightEffect->AddTarget(highlightEntity->GetHighlightTarget());

                highlightEffect->DisplayEffect(m_activeGraphIds.graphCanvasId);

                GetActiveData().second->SetEffect(row, highlightEffect);
            }

            if (const ScriptCanvas::HighlightVariableEffect* highlightvariable = azrtti_cast<const ScriptCanvas::HighlightVariableEffect*>(validationEvent))
            {
                HighlightElementValidationEffect* highlightEffect = aznew HighlightElementValidationEffect();

                AZStd::vector<NodeIdPair> variableNodes;
                EditorGraphRequestBus::EventResult(variableNodes, m_activeGraphIds.scriptCanvasId, &EditorGraphRequests::GetVariableNodes, highlightvariable->GetHighlightVariableId());

                for (auto& variable : variableNodes)
                {
                    highlightEffect->AddTarget(variable.m_scriptCanvasId);
                }

                highlightEffect->DisplayEffect(m_activeGraphIds.graphCanvasId);

                GetActiveData().second->SetEffect(row, highlightEffect);
            }

            if (const ScriptCanvas::GreyOutNodeEffect* greyOutEffect = azrtti_cast<const ScriptCanvas::GreyOutNodeEffect*>(validationEvent))
            {
                m_unusedNodeValidationEffect.AddUnusedNode(greyOutEffect->GetGreyOutNodeId());
            }

            UpdateSelectedText();
        }
    }

    void GraphValidationDockWidget::OnRowDeselected(int row)
    {
        if (auto model = GetActiveModel())
        {
            const ScriptCanvas::ValidationEvent* validationEvent = model->FindItemForRow(row);

            if (const ScriptCanvas::GreyOutNodeEffect* greyOutEffect = azrtti_cast<const ScriptCanvas::GreyOutNodeEffect*>(validationEvent))
            {
                m_unusedNodeValidationEffect.RemoveUnusedNode(greyOutEffect->GetGreyOutNodeId());
            }

            GetActiveData().second->ClearEffect(row);

            UpdateSelectedText();
        }
    }

    void GraphValidationDockWidget::UpdateSelectedText()
    {
        int selectedRowsSize = 0;

        for (const QModelIndex& selectedRow : ui->statusTableView->selectionModel()->selectedRows())
        {
            QModelIndex sourceIndex = m_proxyModel->mapToSource(selectedRow);

            if (auto model = GetActiveModel())
            {
                const ScriptCanvas::ValidationEvent* validationEvent = model->FindItemForRow(sourceIndex.row());

                if (validationEvent->CanAutoFix())
                {
                    selectedRowsSize++;
                }
            }
        }

        if (selectedRowsSize == 0)
        {
            ui->fixSelectedText->setVisible(false);
        }
        else
        {
            ui->fixSelectedText->setVisible(true);
            ui->fixSelectedText->setText(QString("%1 Selected").arg(selectedRowsSize));
        }
    }

    ///////////////////
    // ValidationData
    ///////////////////

    ValidationData::ValidationData()
        : m_model(nullptr)
    {
    }

    ValidationData::ValidationData(GraphCanvas::GraphId graphCanvasId, ScriptCanvas::ScriptCanvasId scriptCanvasId)
        : m_graphCanvasId(graphCanvasId)
    {
        m_model = AZStd::make_unique<GraphValidationModel>();

        if (scriptCanvasId.IsValid())
        {
            ScriptCanvas::StatusRequestBus::Handler::BusConnect(scriptCanvasId);
        }
    }

    ValidationData::~ValidationData()
    {
        ScriptCanvas::StatusRequestBus::Handler::BusDisconnect();

        ClearEffects();
    }

    void ValidationData::ValidateGraph(ScriptCanvas::ValidationResults&)
    {
        // Do nothing, this is asking us to provide the validationEvents, we're only interested
        // in receiving them
    }

    void ValidationData::ReportValidationResults(ScriptCanvas::ValidationResults& validationEvents)
    {
        m_model->Clear();
        m_model->AddEvents(validationEvents);
    }


    ScriptCanvasEditor::ValidationEffect* ValidationData::GetEffect(int row)
    {
        if (m_validationEffects.find(row) != m_validationEffects.end())
        {
            return m_validationEffects[row];
        }

        return nullptr;
    }

    void ValidationData::SetEffect(int row, ValidationEffect* effect)
    {
        if (m_validationEffects.find(row) == m_validationEffects.end())
        {
            m_validationEffects[row] = effect;
        }
    }

    void ValidationData::ClearEffect(int row)
    {
        auto it = m_validationEffects.find(row);
        if (it != m_validationEffects.end())
        {
            m_validationEffects[row]->CancelEffect();
            delete m_validationEffects[row];
            m_validationEffects.erase(it);
        }
    }

    void ValidationData::ClearEffects()
    {
        for (auto it : m_validationEffects)
        {
            delete it.second;
        }
        m_validationEffects.clear();
    }

    void ValidationData::DisplayToast()
    {
        if (m_model->GetValidationResults().GetEvents().empty())
        {
            return;
        }

        GraphCanvas::ViewId viewId;
        GraphCanvas::SceneRequestBus::EventResult(viewId, m_graphCanvasId, &GraphCanvas::SceneRequests::GetViewId);

        AzQtComponents::ToastType toastType;
        AZStd::string titleLabel = "Validation Issue";
        AZStd::string description = "";

        if (m_model->GetValidationResults().HasErrors())
        {
            toastType = AzQtComponents::ToastType::Error;
            description = AZStd::string::format("%i validation error(s) were found.", m_model->GetValidationResults().ErrorCount());
        }
        else
        {
            toastType = AzQtComponents::ToastType::Warning;
            description = AZStd::string::format("%i validation warning(s) were found.", m_model->GetValidationResults().WarningCount());
        }

        AzQtComponents::ToastConfiguration toastConfiguration(toastType, titleLabel.c_str(), description.c_str());

        AzToolsFramework::ToastId validationToastId;

        GraphCanvas::ViewRequestBus::EventResult(validationToastId, viewId, &GraphCanvas::ViewRequests::ShowToastNotification, toastConfiguration);

        AzToolsFramework::ToastNotificationBus::MultiHandler::BusConnect(validationToastId);

    }

    void ValidationData::OnToastInteraction()
    {
        UIRequestBus::Broadcast(&UIRequests::OpenValidationPanel);
    }

    void ValidationData::OnToastDismissed()
    {
        const AzToolsFramework::ToastId* toastId = AzToolsFramework::ToastNotificationBus::GetCurrentBusId();

        if (toastId)
        {
            AzToolsFramework::ToastNotificationBus::MultiHandler::BusDisconnect((*toastId));
        }
    }

#include <Editor/View/Widgets/ValidationPanel/moc_GraphValidationDockWidget.cpp>

}
