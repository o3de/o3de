/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#if !defined(Q_MOC_RUN)
#include <QAbstractItemModel>
#include <QIcon>
#include <QSortFilterProxyModel>

#include <AzQtComponents/Components/StyledDockWidget.h>

#include <GraphCanvas/Editor/AssetEditorBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/ToastBus.h>

#include <ScriptCanvas/Debugger/ValidationEvents/ValidationEvent.h>
#include <ScriptCanvas/Debugger/StatusBus.h>
#endif

namespace Ui
{
    class GraphValidationPanel;
}

namespace ScriptCanvas
{
    class ScopedDataConnectionEvent;
    class InvalidVariableTypeEvent;
    class ScriptEventVersionMismatch;
}

namespace ScriptCanvasEditor
{
    class ValidationEffect
    {
    public:
        AZ_CLASS_ALLOCATOR(ValidationEffect, AZ::SystemAllocator, 0);

        virtual ~ValidationEffect() = default;

        virtual void DisplayEffect(const GraphCanvas::GraphId& graphId) = 0;
        virtual void CancelEffect() = 0;
    };

    class HighlightElementValidationEffect
        : public ValidationEffect
    {
    public:
        AZ_CLASS_ALLOCATOR(HighlightElementValidationEffect, AZ::SystemAllocator, 0);

        HighlightElementValidationEffect();
        HighlightElementValidationEffect(const QColor& color);
        HighlightElementValidationEffect(const GraphCanvas::SceneMemberGlowOutlineConfiguration& glowConfiguration);

        void AddTarget(const AZ::EntityId& targetId);

        void DisplayEffect(const GraphCanvas::GraphId& graphId) override;
        void CancelEffect() override;

    private:
        AZStd::vector< AZ::EntityId > m_targets;

        GraphCanvas::GraphId m_graphId;
        AZStd::vector< GraphCanvas::GraphicsEffectId > m_graphicEffectIds;

        GraphCanvas::SceneMemberGlowOutlineConfiguration m_templateConfiguration;
    };

    class UnusedNodeValidationEffect
        : public ValidationEffect
    {
    public:
        void AddUnusedNode(const AZ::EntityId& graphCanvasNodeId);
        void RemoveUnusedNode(const AZ::EntityId& graphCanvasNodeId);

        void DisplayEffect(const GraphCanvas::GraphId& graphId) override;
        void CancelEffect() override;

    public:

        void ClearStyleSelectors();

        void ApplySelector(const AZ::EntityId& nodeId, AZStd::string_view styleSelector);
        void RemoveSelector(const AZ::EntityId& nodeId);

        bool m_isDirty;

        AZStd::unordered_set< AZ::EntityId > m_unprocessedIds;
        
        AZStd::unordered_set< AZ::EntityId > m_rootUnusedNodes;
        AZStd::unordered_set< AZ::EntityId > m_inactiveNodes;

        AZStd::unordered_map< AZ::EntityId, AZStd::string > m_styleSelectors;
    };

    class GraphValidationModel
        : public QAbstractItemModel
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(GraphValidationModel, AZ::SystemAllocator, 0);

        enum ColumnIndex
        {
            IndexForce = -1,

            Description,
            AutoFix,

            Count
        };
        
        GraphValidationModel();
        ~GraphValidationModel() override;
        
        void RunValidation(const ScriptCanvas::ScriptCanvasId& scriptCanvasId);

        // QAbstractItemModel
        QModelIndex index(int row, int column, const QModelIndex& parent) const override;
        QModelIndex parent(const QModelIndex& index = QModelIndex()) const override;
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        ////
        
        const ScriptCanvas::ValidationEvent* FindItemForIndex(const QModelIndex& index) const;
        const ScriptCanvas::ValidationEvent* FindItemForRow(int row) const;

        const ScriptCanvas::ValidationResults& GetValidationResults() const;
        
    private:
        
        ScriptCanvas::ValidationResults m_validationResults;
        
        QIcon m_errorIcon;
        QIcon m_warningIcon;
        QIcon m_messageIcon;

        QIcon m_autoFixIcon;
    };
    
    class GraphValidationSortFilterProxyModel
        : public QSortFilterProxyModel
    {
        Q_OBJECT
    public:

        AZ_CLASS_ALLOCATOR(GraphValidationSortFilterProxyModel, AZ::SystemAllocator, 0);
    
        GraphValidationSortFilterProxyModel();
    
        bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

        void SetFilter(const QString& filterString);
        
        void SetSeverityFilter(ScriptCanvas::ValidationSeverity severityFilter);
        ScriptCanvas::ValidationSeverity GetSeverityFilter() const;

        bool IsShowingErrors();
        bool IsShowingWarnings();
    
    private:
        
        ScriptCanvas::ValidationSeverity m_severityFilter;

        QString m_filter;
        QRegExp m_regex;
    };

    class GraphValidationDockWidget
        : public AzQtComponents::StyledDockWidget
        , public GraphCanvas::AssetEditorNotificationBus::Handler
        , public GraphCanvas::SceneNotificationBus::Handler
        , public GraphCanvas::ToastNotificationBus::MultiHandler
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(GraphValidationDockWidget, AZ::SystemAllocator, 0);
        
        GraphValidationDockWidget(QWidget* parent = nullptr);
        ~GraphValidationDockWidget();
        
        // GraphCanvas::AssetEditorNotificationBus::Handler
        void OnActiveGraphChanged(const GraphCanvas::GraphId& graphCanvasGraphId) override;
        ////

        // GrpahCanvas::SceneNotificationBus
        void OnSelectionChanged() override;

        void OnConnectionDragBegin() override;
        ////

        // ToastNotification
        void OnToastInteraction() override;
        void OnToastDismissed() override;
        ////

        bool HasValidationIssues() const;
        
    public slots:
    
        void OnRunValidator(bool displayAsNotification = false);
        void OnShowErrors();
        void OnShowWarnings();
        
        void OnTableSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
        void FocusOnEvent(const QModelIndex& modelIndex);

        void TryAutoFixEvent(const QModelIndex& modelIndex);
        void FixSelected();

        void OnSeverityFilterChanged();
        void OnFilterChanged(const QString& filterString);

    private:

        // AutoFixes
        void AutoFixEvent(const ScriptCanvas::ValidationEvent* validationEvent);
        void AutoFixScopedDataConnection(const ScriptCanvas::ScopedDataConnectionEvent* connectionEvent);
        void AutoFixDeleteInvalidVariables(const ScriptCanvas::InvalidVariableTypeEvent* invalidVariableEvent);
        void AutoFixScriptEventVersionMismatch(const ScriptCanvas::ScriptEventVersionMismatch* scriptEventMismatchEvent);
        ////

        void UpdateText();

        void OnRowSelected(int row);
        void OnRowDeselected(int row);

        void UpdateSelectedText();
    
        GraphValidationModel* m_model;
        GraphValidationSortFilterProxyModel* m_proxyModel;
    
        ScriptCanvas::ScriptCanvasId    m_scriptCanvasId;
        GraphCanvas::GraphId            m_graphCanvasGraphId;

        UnusedNodeValidationEffect m_unusedNodeValidationEffect;
        AZStd::unordered_map< int, ValidationEffect* > m_validationEffects;
        
        AZStd::unique_ptr<Ui::GraphValidationPanel> ui;
    };
}
