/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QAbstractItemModel>
#include <QIcon>
#include <QSortFilterProxyModel>

#include <AzCore/Debug/TraceMessageBus.h>

#include <AzQtComponents/Components/StyledDockWidget.h>
#include <AzToolsFramework/UI/Notifications/ToastBus.h>

#include <GraphCanvas/Editor/AssetEditorBus.h>
#include <GraphCanvas/Components/SceneBus.h>

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
    //! Visual effect interface
    class ValidationEffect
    {
    public:
        AZ_CLASS_ALLOCATOR(ValidationEffect, AZ::SystemAllocator);

        virtual ~ValidationEffect() = default;

        virtual void DisplayEffect(const GraphCanvas::GraphId& graphId) = 0;
        virtual void CancelEffect() = 0;
    };

    //! Highlights the border of a graph element to display its status
    class HighlightElementValidationEffect
        : public ValidationEffect
    {
    public:
        AZ_CLASS_ALLOCATOR(HighlightElementValidationEffect, AZ::SystemAllocator);

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

    //! Effect used to show when a node is unused
    class UnusedNodeValidationEffect
        : public ValidationEffect
    {
    public:
        AZ_CLASS_ALLOCATOR(UnusedNodeValidationEffect, AZ::SystemAllocator)
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
        AZ_CLASS_ALLOCATOR(GraphValidationModel, AZ::SystemAllocator);

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
        void AddEvents(ScriptCanvas::ValidationResults& validationEvents);
        void Clear();

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

        AZ_CLASS_ALLOCATOR(GraphValidationSortFilterProxyModel, AZ::SystemAllocator);
    
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

    //! Owns the model for each currently opened graph
    class ValidationData
        : private ScriptCanvas::StatusRequestBus::Handler
        , private AzToolsFramework::ToastNotificationBus::MultiHandler
    {
    public:
        AZ_CLASS_ALLOCATOR(ValidationData, AZ::SystemAllocator);

        ValidationData();
        ValidationData(GraphCanvas::GraphId graphCanvasId, ScriptCanvas::ScriptCanvasId scriptCanvasId);
        ~ValidationData() override;

        // ScriptCanvas::StatusRequestBus
        void ValidateGraph(ScriptCanvas::ValidationResults& validationEvents) override;
        void ReportValidationResults(ScriptCanvas::ValidationResults& validationEvents) override;
        ////

        ValidationEffect* GetEffect(int row);
        void SetEffect(int row, ValidationEffect* effect);
        void ClearEffect(int row);
        void ClearEffects();

        GraphValidationModel* GetModel() { return m_model.get(); }

        void DisplayToast();

    private:

        // ToastNotification
        void OnToastInteraction() override;
        void OnToastDismissed() override;
        ////

        AZStd::unique_ptr<GraphValidationModel> m_model;

        using ValidationEffectMap = AZStd::unordered_map< int, ValidationEffect* >;
        ValidationEffectMap m_validationEffects;

        GraphCanvas::GraphId m_graphCanvasId;

    };

    //! Displays warnings or errors related for a Script Canvas graph
    class GraphValidationDockWidget
        : public AzQtComponents::StyledDockWidget
        , public GraphCanvas::AssetEditorNotificationBus::Handler
        , public GraphCanvas::SceneNotificationBus::Handler
        , public AzToolsFramework::ToastNotificationBus::Handler
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(GraphValidationDockWidget, AZ::SystemAllocator);
        
        GraphValidationDockWidget(QWidget* parent = nullptr);
        ~GraphValidationDockWidget();
        
        // GraphCanvas::AssetEditorNotificationBus::Handler
        void OnActiveGraphChanged(const GraphCanvas::GraphId& graphCanvasGraphId) override;
        ////

        // GrpahCanvas::SceneNotificationBus
        void OnSelectionChanged() override;

        void OnConnectionDragBegin() override;
        ////

        bool HasValidationIssues() const;

    public Q_SLOTS:
    
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
        void UpdateSelectedText();

        void OnRowSelected(int row);
        void OnRowDeselected(int row);

        void Refresh();

        GraphValidationSortFilterProxyModel* m_proxyModel;
        AZStd::unique_ptr<Ui::GraphValidationPanel> ui;

        UnusedNodeValidationEffect m_unusedNodeValidationEffect;

        //! Every graph will store its own validation model, this makes it possible to display the latest
        //! validation state even if the active graph changes
        using GraphModelPair = AZStd::pair<ScriptCanvas::ScriptCanvasId, AZStd::unique_ptr<ValidationData>>;
        AZStd::unordered_map<GraphCanvas::GraphId, GraphModelPair> m_models;

        const GraphValidationModel* GetActiveModel() const;
        GraphModelPair& GetActiveData();

        // We use the IDs of the active graph for different functions, so we keep both for easy access
        struct IdPair
        {
            GraphCanvas::GraphId graphCanvasId;
            ScriptCanvas::ScriptCanvasId scriptCanvasId;
        };
        IdPair m_activeGraphIds;

    };
}
