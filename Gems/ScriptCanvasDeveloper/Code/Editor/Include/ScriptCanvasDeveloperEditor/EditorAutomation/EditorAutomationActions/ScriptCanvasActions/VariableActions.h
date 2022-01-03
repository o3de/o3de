/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QAbstractItemView>
#include <QLineEdit>
#include <QRect>
#include <QPoint>
#include <QString>
#include <QWidget>

#include <GraphCanvas/Types/Endpoint.h>
#include <GraphCanvas/Types/Types.h>
#include <GraphCanvas/Widgets/NodePalette/NodePaletteWidget.h>

#include <ScriptCanvas/Core/Datum.h>
#include <ScriptCanvas/Variable/VariableCore.h>
#include <ScriptCanvas/Variable/VariableBus.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/GenericActions.h>

namespace ScriptCanvasDeveloper
{
    /**
        EditorAutomationAction that will create a variable in the active graph using the specified creation action
    */
    class CreateVariableAction
        : public CompoundAction
        , public ScriptCanvas::GraphVariableManagerNotificationBus::Handler
    {
    public:

        enum class CreationType
        {
            Palette,
            AutoComplete,
            Programmatic
        };

        AZ_CLASS_ALLOCATOR(CreateVariableAction, AZ::SystemAllocator, 0);
        AZ_RTTI(CreateVariableAction, "{0C2A4B3C-FCE3-4611-BACB-4651B40CD715}", CompoundAction);

        CreateVariableAction(ScriptCanvas::Data::Type dataType, CreationType creationType = CreationType::AutoComplete);
        CreateVariableAction(ScriptCanvas::Data::Type dataType, QString variableName, CreationType creationType = CreationType::AutoComplete);
        ~CreateVariableAction() override = default;

        void SetErrorOnNameMisMatch(bool enabled);

        void SetupAction() override;

        ScriptCanvas::VariableId GetVariableId() const;

        // GraphVariableManagerNotificationBus
        void OnVariableAddedToGraph(const ScriptCanvas::VariableId& variableId, AZStd::string_view variableName) override;
        ////

        ActionReport GenerateReport() const override;

    protected:

        void OnActionsComplete() override;

    private:

        CreationType m_creationType = CreationType::AutoComplete;

        bool     m_errorOnNameMismatch = true;
        QString m_variableName;

        ScriptCanvas::Data::Type m_dataType;
        QString m_typeName;

        ScriptCanvas::ScriptCanvasId m_scriptCanvasId;
        ScriptCanvas::VariableId m_variableId;
    };

    /**
        EditorAutomationAction that will create a get/set variable node from the variable palette
    */
    class CreateVariableNodeFromGraphPalette
        : public CompoundAction
        , public GraphCanvas::SceneNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(CreateVariableNodeFromGraphPalette, AZ::SystemAllocator, 0);
        AZ_RTTI(CreateVariableNodeFromGraphPalette, "{B0E36790-2BED-4338-A462-7F302AFA5671}", CompoundAction);

        CreateVariableNodeFromGraphPalette(const AZStd::string& variableName, const GraphCanvas::GraphId& graphId, QPoint scenePoint, Qt::KeyboardModifier = Qt::KeyboardModifier::ShiftModifier);
        ~CreateVariableNodeFromGraphPalette() override = default;

        bool IsMissingPrecondition() override;
        EditorAutomationAction* GenerateMissingPreconditionAction() override;

        void SetupAction() override;

        // GraphCanvas::SceneNotificationBus
        void OnNodeAdded(const AZ::EntityId& nodeId, bool isPaste) override;
        ////

        AZ::EntityId GetCreatedNodeId() const;

        ActionReport GenerateReport() const override;

    protected:

        void OnActionsComplete() override;

    private:

        QTableView* m_graphPalette = nullptr;
        QLineEdit* m_textFilter = nullptr;

        bool m_isShowingPalette = false;
        bool m_isFiltered = false;
        bool m_indexIsVisible = false;
        bool m_scenePointVisible = false;

        QModelIndex   m_displayIndex;

        AZStd::string m_variableName;

        GraphCanvas::GraphId m_graphId;
        GraphCanvas::ViewId m_viewId;
        QPoint m_scenePoint;

        AZ::EntityId m_createdNodeId;

        Qt::KeyboardModifier m_modifier;
    };

    /**
        EditorAutomationAction that will ensure that VariableWidget is showing the active GraphVariables list
    */
    class ShowGraphVariablesAction
        : public CompoundAction
    {
    public:
        AZ_CLASS_ALLOCATOR(ShowGraphVariablesAction, AZ::SystemAllocator, 0);
        AZ_RTTI(ShowGraphVariablesAction, "{853F34EB-F8AB-47E3-A601-35091DC23E11}", CompoundAction);

        ShowGraphVariablesAction() = default;
        ~ShowGraphVariablesAction() override = default;

        void SetupAction() override;

        ActionReport GenerateReport() const override;
    };
}
