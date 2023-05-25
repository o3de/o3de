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
#include <QRegExp>
#include <QString>
#include <QSortFilterProxyModel>
#include <QTableView>

#include <AzCore/Component/EntityId.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>

#include <ScriptCanvas/Bus/NodeIdPair.h>
#include <ScriptCanvas/Variable/VariableBus.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Widgets/StyledItemDelegates/IconDecoratedNameDelegate.h>
#endif

namespace ScriptCanvasEditor
{
    class GraphVariablesModel
        : public QAbstractTableModel
        , public ScriptCanvas::GraphVariableManagerNotificationBus::Handler
        , public ScriptCanvas::VariableNotificationBus::MultiHandler
    {
        Q_OBJECT
    public:

        enum ColumnIndex
        {
            Name,
            Type,
            DefaultValue,
            Scope,
            InitialValueSource,
            Count
        };

        static const char* m_columnNames[static_cast<int>(ColumnIndex::Count)];

        enum CustomRole
        {
            VarIdRole = Qt::UserRole
        };

        static const char* GetMimeType() { return "o3de/x-scriptcanvas-varpanel"; }

        AZ_CLASS_ALLOCATOR(GraphVariablesModel, AZ::SystemAllocator);
        GraphVariablesModel(QObject* parent = nullptr);
        ~GraphVariablesModel();

        // QAbstractTableModel
        int columnCount(const QModelIndex &parent = QModelIndex()) const override;
        int rowCount(const QModelIndex &parent = QModelIndex()) const override;

        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        bool setData(const QModelIndex& index, const QVariant& value, int role) override;

        Qt::ItemFlags flags(const QModelIndex &index) const override;

        QStringList mimeTypes() const override;
        QMimeData* mimeData(const QModelIndexList &indexes) const override;
        ////

        void SetActiveScene(const ScriptCanvas::ScriptCanvasId& scriptCanvasId);
        ScriptCanvas::ScriptCanvasId GetScriptCanvasId() const;

        // ScriptCanvas::GraphVariableManagerNotificationBus
        void OnVariableAddedToGraph(const ScriptCanvas::VariableId& variableId, AZStd::string_view variableName) override;
        void OnVariableRemovedFromGraph(const ScriptCanvas::VariableId& variableId, AZStd::string_view variableName) override;
        void OnVariableNameChangedInGraph(const ScriptCanvas::VariableId& variableId, AZStd::string_view variableName) override;
        ////

        // ScriptCanvas::VariableRuntimeNotificationBus
        void OnVariableValueChanged() override;
        void OnVariableScopeChanged() override;
        void OnVariableInitialValueSourceChanged() override;
        void OnVariablePriorityChanged() override;
        ////

        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;


        ScriptCanvas::VariableId FindVariableIdForIndex(const QModelIndex& index) const;
        ScriptCanvas::GraphScopedVariableId FindScopedVariableIdForIndex(const QModelIndex& index) const;

        int FindRowForVariableId(const ScriptCanvas::VariableId& variableId) const;

        bool IsFunction() const;

    signals:
        void VariableAdded(QModelIndex modelIndex);

    private:

        bool IsEditableType(ScriptCanvas::Data::Type scriptCanvasDataType) const;

        void PopulateSceneVariables();

        AZStd::vector<ScriptCanvas::GraphScopedVariableId> m_variableIds;
        ScriptCanvas::ScriptCanvasId m_scriptCanvasId;
    };

    class GraphVariablesModelSortFilterProxyModel
        : public QSortFilterProxyModel
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(GraphVariablesModelSortFilterProxyModel, AZ::SystemAllocator);
        GraphVariablesModelSortFilterProxyModel(QObject* parent);

        bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
        bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

        void SetFilter(const QString& filter);

    private:
        QString m_filter;
        QRegExp m_filterRegex;

        ScriptCanvas::GraphVariable::Comparator m_variableComparator;
    };

    class GraphVariablesTableView
        : public QTableView
        , public GraphCanvas::SceneNotificationBus::Handler
    {
        Q_OBJECT
    public:
        static bool HasCopyVariableData();
        static void CopyVariableToClipboard(const ScriptCanvas::ScriptCanvasId& scriptCanvasId, const ScriptCanvas::VariableId& variableId);
        static bool HandleVariablePaste(const ScriptCanvas::ScriptCanvasId& scriptCanvasId);

        AZ_CLASS_ALLOCATOR(GraphVariablesTableView, AZ::SystemAllocator);
        explicit GraphVariablesTableView(QWidget* parent);
        ~GraphVariablesTableView();

        void SetActiveScene(const ScriptCanvas::ScriptCanvasId& scriptCanvasId);
        void SetFilter(const QString& filterString);

        void EditVariableName(ScriptCanvas::VariableId variableId);

        // QObject
        void hideEvent(QHideEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        ////

        // QTableView
        void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;
        ////

        // GraphCanvas::SceneNotifications
        void OnSelectionChanged() override;
        ////

        void ApplyPreferenceSort();

        void ResizeColumns();

    public slots:
        void OnVariableAdded(QModelIndex modelIndex);
        void OnDeleteSelected();
        void OnCopySelected();
        void OnPaste();
        void OnDuplicate();

        void SetCycleTarget(ScriptCanvas::VariableId variableId);
        void CycleToNextVariableReference();
        void CycleToPreviousVariableReference();

    signals:
        void SelectionChanged(const AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds);
        void DeleteVariables(const AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds);

    private:

        void ConfigureHelper();

        AZ::EntityId                             m_graphCanvasGraphId;
        ScriptCanvas::ScriptCanvasId             m_scriptCanvasId;
        GraphVariablesModelSortFilterProxyModel* m_proxyModel;
        GraphVariablesModel*                     m_model;

        QAction*                             m_nextInstanceAction;
        QAction*                             m_previousInstanceAction;

        ScriptCanvas::VariableId             m_cyclingVariableId;
        GraphCanvas::NodeFocusCyclingHelper  m_cyclingHelper;
    };
}
