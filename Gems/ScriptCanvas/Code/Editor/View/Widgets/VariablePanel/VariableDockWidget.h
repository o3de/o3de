/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/PlatformDef.h>
AZ_PUSH_DISABLE_WARNING(4244 4251 4800, "-Wunknown-warning-option")
#include <QAbstractListModel>
#include <QAbstractItemView>
#include <QListView>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QTimer>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QFocusEvent>
#include <QMenu>
AZ_POP_DISABLE_WARNING

#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Casting/numeric_cast.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <AzQtComponents/Components/StyledDockWidget.h>

#include <GraphCanvas/Editor/AssetEditorBus.h>
#include <GraphCanvas/Components/GraphCanvasPropertyBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/Nodes/NodeTitleBus.h>
#include <GraphCanvas/Widgets/GraphCanvasTreeItem.h>
#include <GraphCanvas/Widgets/GraphCanvasTreeModel.h>

#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Variable/VariableBus.h>
#include <ScriptCanvas/Variable/GraphVariable.h>

#include <Editor/View/Widgets/VariablePanel/VariablePaletteTableView.h>
#include <Editor/View/Widgets/VariablePanel/GraphVariablesTableView.h>
#endif

class QAction;
class QLineEdit;
class QPushButton;

namespace Ui
{
    class VariableDockWidget;
}

namespace ScriptCanvasEditor
{
    class VariablePropertiesComponent
        : public GraphCanvas::GraphCanvasPropertyComponent
        , protected ScriptCanvas::VariableNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(VariablePropertiesComponent, "{885F276B-9633-42F7-85BD-10869E606873}", GraphCanvasPropertyComponent);

        static void Reflect(AZ::ReflectContext*);
        static AZ::Entity* CreateVariablePropertiesEntity();

        VariablePropertiesComponent();
        ~VariablePropertiesComponent() = default;

        const char* GetTitle();

        void SetVariable(ScriptCanvas::GraphVariable* variable);

    private:
        void OnNameChanged();

        // VariableNotificationBus::Handler
        void OnVariableRemoved() override;
        void OnVariableRenamed(AZStd::string_view variableName) override;        
        void OnVariableScopeChanged() override;

        void OnVariableValueChanged() override;
        ////

        AZStd::string m_variableName;
        ScriptCanvas::GraphVariable* m_variable;
        ScriptCanvas::ScriptCanvasId m_scriptCanvasGraphId;

        AZStd::string m_componentTitle;
    };

    class VariableDockWidget;

    class VariablePanelContextMenu
        : public QMenu
    {
    public:
        VariablePanelContextMenu(VariableDockWidget* contextMenu, const ScriptCanvas::ScriptCanvasId& scriptCanvasExecutionId
            , ScriptCanvas::VariableId varId, QPoint position);
    };

    class VariableDockWidget
        : public AzQtComponents::StyledDockWidget
        , public GraphCanvas::AssetEditorNotificationBus::Handler
        , public AzToolsFramework::EditorEvents::Bus::Handler
        , public VariableAutomationRequestBus::Handler
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(VariableDockWidget, AZ::SystemAllocator);

        static AZStd::string ConstructDefaultVariableName(AZ::u32 variableCounter);
        static AZStd::string FindDefaultVariableName(const ScriptCanvas::ScriptCanvasId& scriptCanvasGraphId);

        VariableDockWidget(QWidget* parent = nullptr);
        ~VariableDockWidget();

        void PopulateVariablePalette(const AZStd::unordered_set< AZ::Uuid >& objectTypes);

        // GraphCanvas::AssetEditorNotificationBus::Handler
        void OnActiveGraphChanged(const GraphCanvas::GraphId& graphCanvasGraphId) override;
        ////

        // VariableAutomationRequestBus
        AZStd::vector< ScriptCanvas::Data::Type > GetPrimitiveTypes() const override;
        AZStd::vector< ScriptCanvas::Data::Type > GetBehaviorContextObjectTypes() const override;
        AZStd::vector< ScriptCanvas::Data::Type > GetMapTypes() const override;
        AZStd::vector< ScriptCanvas::Data::Type > GetArrayTypes() const override;

        bool IsShowingVariablePalette() const override;
        bool IsShowingGraphVariables() const override;

        QPushButton* GetCreateVariableButton() const override;
        QTableView* GetGraphPaletteTableView() const override;
        QTableView* GetVariablePaletteTableView() const override;

        QLineEdit* GetVariablePaletteFilter() const override;
        QLineEdit* GetGraphVariablesFilter() const override;
        ////

        // AzToolsFramework::EditorEvents::Bus
        void OnEscape() override;
        ////

        // QWidget
        void focusOutEvent(QFocusEvent* focusEvent) override;
        ////

        const ScriptCanvas::ScriptCanvasId& GetActiveScriptCanvasId() const;

        bool IsValidVariableType(const ScriptCanvas::Data::Type& dataType) const;

    public slots:
        void OnCreateVariable(ScriptCanvas::Data::Type varType);
        void OnCreateNamedVariable(const AZStd::string& variableName, ScriptCanvas::Data::Type varType);
        void OnSelectionChanged(const AZStd::unordered_set< ScriptCanvas::VariableId>& variableIds);
        void OnDuplicateVariable(const ScriptCanvas::VariableId& variableId);
        void OnDeleteVariables(const AZStd::unordered_set< ScriptCanvas::VariableId>& variableIds);
        void OnHighlightVariables(const AZStd::unordered_set< ScriptCanvas::VariableId>& variableIds);

        void OnRemoveUnusedVariables();
        void OnConfigureVariable(const ScriptCanvas::VariableId& variableId, QPoint position);

    Q_SIGNALS:
        void OnVariableSelectionChanged(const AZStd::vector<AZ::EntityId>& variableIds);

    private:

        
        void ShowVariablePalette();
        void ShowGraphVariables();

        void FocusOnSearchFilter();

        void ClearFilter();
        void UpdateFilter();

        void OnReturnPressed();
        void OnQuickFilterChanged(const QString &text);

        void RefreshModel();

        void OnAddVariableButton();
        void OnContextMenuRequested(const QPoint &pos);

        bool CanDeleteVariable(const ScriptCanvas::VariableId& variableId);

        VariablePropertiesComponent* AllocateComponent(const ScriptCanvas::VariableId& variableId);
        void ReleaseComponent(const ScriptCanvas::VariableId& variableId);
        void ResetPool();

        AZStd::unordered_map< ScriptCanvas::VariableId, VariablePropertiesComponent* > m_usedElements;
        AZStd::vector< VariablePropertiesComponent* > m_unusedPool;

        AZStd::vector< AZStd::unique_ptr<AZ::Entity> > m_propertyHelpers;

        ScriptCanvas::ScriptCanvasId  m_scriptCanvasId;

        AZ::EntityId m_graphCanvasGraphId;

        AZStd::unique_ptr<Ui::VariableDockWidget> ui;

        QTimer m_filterTimer;
    };
}
