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

#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Casting/numeric_cast.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <AzQtComponents/Components/StyledDialog.h>

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
    class VariableConfigurationWidget;
}

namespace ScriptCanvasEditor
{
    class VariableConfigurationWidget
        : public AzQtComponents::StyledDialog
        , public AzToolsFramework::EditorEvents::Bus::Handler
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(VariableConfigurationWidget, AZ::SystemAllocator);

        VariableConfigurationWidget
            ( const ScriptCanvas::ScriptCanvasId& scriptCanvasId
            , const VariablePaletteRequests::VariableConfigurationInput& input
            , QWidget* parent = nullptr);

        ~VariableConfigurationWidget();

        void PopulateVariablePalette(const AZStd::unordered_set< AZ::Uuid >& objectTypes);

        // AzToolsFramework::EditorEvents::Bus
        void OnEscape() override;
        ////

        // QWidget
        void focusOutEvent(QFocusEvent* focusEvent) override;
        ////

        AZ::Uuid GetSelectedType() const;
        AZStd::string GetSlotName() const;
        void SetSlotName(AZStd::string);

        const ScriptCanvas::ScriptCanvasId& GetActiveScriptCanvasId() const;

    public Q_SLOTS:
        void OnCreateVariable(ScriptCanvas::Data::Type varType);

    Q_SIGNALS:
        void OnVariableSelectionChanged(const AZStd::vector<AZ::EntityId>& variableIds);

    private:

        void ShowVariablePalette();

        void FocusOnSearchFilter();

        void ClearFilter();
        void UpdateFilter();

        void OnReturnPressed();
        void OnQuickFilterChanged(const QString& text);
        void OnNameChanged(const QString& text);

        void OnContextMenuRequested(const QPoint& pos);

        AZStd::vector< AZStd::unique_ptr<AZ::Entity> > m_propertyHelpers;

        ScriptCanvas::ScriptCanvasId  m_scriptCanvasId;

        AZ::EntityId m_graphCanvasGraphId;

        AZ::Uuid m_selectedType;
        AZStd::string m_slotName;
        const VariablePaletteRequests::VariableConfigurationInput& m_input;

        AZStd::unique_ptr<Ui::VariableConfigurationWidget> ui;

        QTimer m_filterTimer;
    };
}
