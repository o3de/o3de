/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "precompiled.h"

#include <QApplication>
#include <QPushButton>
#include <QTableView>
#include <QToolButton>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/Slots/Data/DataSlotBus.h>
#include <GraphCanvas/Components/ViewBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>

#include <GraphCanvas/Widgets/NodePalette/Model/NodePaletteSortFilterProxyModel.h>
#include <GraphCanvas/Widgets/NodePalette/NodePaletteTreeView.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>
#include <GraphCanvas/Utils/ConversionUtils.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>
#include <Editor/View/Widgets/VariablePanel/VariableDockWidget.h>

#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/VariableActions.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/EditorMouseActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/EditorKeyActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/EditorViewActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/WidgetActions.h>

#if !defined(AZ_COMPILER_MSVC)
#ifndef VK_RETURN
#define VK_RETURN 0x0D
#endif
#ifndef VK_ESCAPE 
#define VK_ESCAPE 0x1B
#endif
#endif

namespace ScriptCanvasDeveloper
{
    /////////////////////////
    // CreateVariableAction
    /////////////////////////

    CreateVariableAction::CreateVariableAction(ScriptCanvas::Data::Type dataType, CreationType creationType)
        : m_creationType(creationType)
        , m_dataType(dataType)
        , m_typeName(ScriptCanvas::Data::GetName(dataType).c_str())
    {
    }

    CreateVariableAction::CreateVariableAction(ScriptCanvas::Data::Type dataType, QString variableName, CreationType creationType)
        : m_creationType(creationType)
        , m_variableName(variableName)
        , m_dataType(dataType)
        , m_typeName(ScriptCanvas::Data::GetName(dataType).c_str())
    {

    }

    void CreateVariableAction::SetErrorOnNameMisMatch(bool enabled)
    {
        m_errorOnNameMismatch = enabled;
    }

    void CreateVariableAction::SetupAction()
    {
        ScriptCanvasEditor::GeneralRequestBus::BroadcastResult(m_scriptCanvasId, &ScriptCanvasEditor::GeneralRequests::GetActiveScriptCanvasId);

        ClearActionQueue();

        if (m_creationType != CreationType::Programmatic)
        {
            bool isShowingCreatePalette = false;
            ScriptCanvasEditor::VariableAutomationRequestBus::BroadcastResult(isShowingCreatePalette, &ScriptCanvasEditor::VariableAutomationRequests::IsShowingVariablePalette);

            if (!isShowingCreatePalette)
            {
                QPushButton* targetButton = nullptr;
                ScriptCanvasEditor::VariableAutomationRequestBus::BroadcastResult(targetButton, &ScriptCanvasEditor::VariableAutomationRequests::GetCreateVariableButton);

                QPoint targetPoint = targetButton->mapToGlobal(targetButton->rect().center());

                AddAction(aznew MouseClickAction(Qt::MouseButton::LeftButton, targetPoint));
                AddAction(aznew ProcessUserEventsAction());
            }

            QLineEdit* searchFilter = nullptr;
            ScriptCanvasEditor::VariableAutomationRequestBus::BroadcastResult(searchFilter, &ScriptCanvasEditor::VariableAutomationRequests::GetVariablePaletteFilter);

            if (searchFilter)
            {
                AddAction(aznew WriteToLineEditAction(searchFilter, m_typeName));
            }
        }

        switch (m_creationType)
        {
        case CreationType::Palette:
        {
            QTableView* variableView = nullptr;
            ScriptCanvasEditor::VariableAutomationRequestBus::BroadcastResult(variableView, &ScriptCanvasEditor::VariableAutomationRequests::GetVariablePaletteTableView);
            
            AddAction(aznew ProcessUserEventsAction(AZStd::chrono::milliseconds(500)));
            AddAction(aznew MoveMouseToViewRowAction(variableView, 0));
            AddAction(aznew MouseClickAction(Qt::MouseButton::LeftButton));
        }
            break;
        case CreationType::AutoComplete:
        {
            AddAction(aznew TypeCharAction(VK_RETURN));
        }
            break;
        case CreationType::Programmatic:
        {
            bool nameAvailable = false;
            AZStd::string variableName;

            if (!m_variableName.isEmpty())
            {
                variableName = m_variableName.toUtf8().data();
                ScriptCanvas::GraphVariableManagerRequestBus::EventResult(nameAvailable, m_scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::IsNameAvailable, variableName);
            }

            if (!nameAvailable)
            {
                int variableCounter = 1;

                do
                {
                    ScriptCanvasEditor::SceneCounterRequestBus::EventResult(variableCounter, m_scriptCanvasId, &ScriptCanvasEditor::SceneCounterRequests::GetNewVariableCounter);

                    // From VariableDockWidget, Should always be in sync with that.
                    variableName = AZStd::string::format("Variable %u", variableCounter);

                    ScriptCanvas::GraphVariableManagerRequestBus::EventResult(nameAvailable, m_scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::IsNameAvailable, variableName);
                } while (!nameAvailable);
            }

            ScriptCanvas::Datum datum(m_dataType, ScriptCanvas::Datum::eOriginality::Original);

            AZ::Outcome<ScriptCanvas::VariableId, AZStd::string> outcome = AZ::Failure(AZStd::string());
            ScriptCanvas::GraphVariableManagerRequestBus::EventResult(outcome, m_scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::AddVariable, variableName, datum, false);

            if (outcome)
            {
                m_variableId = outcome.GetValue();
            }
        }
            break;
        default:
            break;
        }

        AddAction(aznew ProcessUserEventsAction());

        if (m_creationType != CreationType::Programmatic)
        {
            if (!m_variableName.isEmpty())
            {
                AddAction(aznew TypeStringAction(m_variableName));
                AddAction(aznew ProcessUserEventsAction());
                AddAction(aznew TypeCharAction(VK_RETURN));
                AddAction(aznew ProcessUserEventsAction());
            }
            else
            {
                AddAction(aznew TypeCharAction(VK_ESCAPE));
                AddAction(aznew ProcessUserEventsAction());
            }
        }

        ScriptCanvas::GraphVariableManagerNotificationBus::Handler::BusConnect(m_scriptCanvasId);

        CompoundAction::SetupAction();
    }

    ScriptCanvas::VariableId CreateVariableAction::GetVariableId() const
    {
        return m_variableId;
    }

    void CreateVariableAction::OnVariableAddedToGraph(const ScriptCanvas::VariableId& variableId, AZStd::string_view /*variableName*/)
    {
        m_variableId = variableId;
    }

    ActionReport CreateVariableAction::GenerateReport() const
    {
        if (!m_variableId.IsValid())
        {
            return AZ::Failure<AZStd::string>(AZStd::string::format("Failed to create Variable with type %s", ScriptCanvas::Data::GetName(m_dataType).c_str()));
        }
        else if (!m_variableName.isEmpty() && m_errorOnNameMismatch)
        {
            ScriptCanvas::GraphVariable* graphVariable = nullptr;
            ScriptCanvas::GraphVariableManagerRequestBus::EventResult(graphVariable, m_scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::FindVariableById, m_variableId);

            if (graphVariable)
            {
                if (m_variableName.compare(graphVariable->GetVariableName().data(), Qt::CaseInsensitive) != 0)
                {
                    return AZ::Failure<AZStd::string>(AZStd::string::format("Failed to name Variable %s", m_variableName.toUtf8().data()));
                }
            }
        }

        return CompoundAction::GenerateReport();
    }

    void CreateVariableAction::OnActionsComplete()
    {
        ScriptCanvas::GraphVariableManagerNotificationBus::Handler::BusDisconnect();
    }

    ///////////////////////////////////////
    // CreateVariableNodeFromGraphPalette
    ///////////////////////////////////////

    CreateVariableNodeFromGraphPalette::CreateVariableNodeFromGraphPalette(const AZStd::string& variableName, const GraphCanvas::GraphId& graphId, QPoint scenePoint, Qt::KeyboardModifier modifier)
        : m_variableName(variableName)
        , m_graphId(graphId)
        , m_modifier(modifier)
        , m_scenePoint(scenePoint)
    {
        ScriptCanvasEditor::VariableAutomationRequestBus::BroadcastResult(m_graphPalette, &ScriptCanvasEditor::VariableAutomationRequests::GetGraphPaletteTableView);
        ScriptCanvasEditor::VariableAutomationRequestBus::BroadcastResult(m_textFilter, &ScriptCanvasEditor::VariableAutomationRequests::GetGraphVariablesFilter);

        GraphCanvas::SceneRequestBus::EventResult(m_viewId, m_graphId, &GraphCanvas::SceneRequests::GetViewId);
    }

    bool CreateVariableNodeFromGraphPalette::IsMissingPrecondition()
    {
        ScriptCanvasEditor::VariableAutomationRequestBus::BroadcastResult(m_isShowingPalette, &ScriptCanvasEditor::VariableAutomationRequests::IsShowingGraphVariables);

        if (m_textFilter)
        {
            m_isFiltered = (m_textFilter->text().compare(m_variableName.c_str(), Qt::CaseInsensitive) == 0);
        }
        else
        {
            m_isFiltered = true;
        }

        m_indexIsVisible = false;
        m_displayIndex = QModelIndex();

        if (m_isFiltered)
        {
            int rowCount = m_graphPalette->model()->rowCount();

            for (int i = 0; i < rowCount; ++i)
            {
                m_displayIndex = m_graphPalette->model()->index(i, 0);

                if (m_displayIndex.isValid())
                {
                    QVariant variant = m_graphPalette->model()->data(m_displayIndex, Qt::DisplayRole);
                    QString name = variant.toString();

                    if (name.compare(m_variableName.c_str(), Qt::CaseInsensitive) == 0)
                    {
                        break;
                    }
                }
            }

            QRegion region = m_graphPalette->visibleRegion();
            QRect boundingRegion = region.boundingRect();
            m_indexIsVisible = region.contains(m_graphPalette->visualRect(m_displayIndex).center());
        }

        m_scenePointVisible = false;

        QRectF viewableArea;
        GraphCanvas::ViewRequestBus::EventResult(viewableArea, m_viewId, &GraphCanvas::ViewRequests::GetViewableAreaInSceneCoordinates);

        m_scenePointVisible = viewableArea.contains(m_scenePoint);

        return !m_isShowingPalette || !m_isFiltered || !m_indexIsVisible || ! m_scenePointVisible;
    }

    EditorAutomationAction* CreateVariableNodeFromGraphPalette::GenerateMissingPreconditionAction()
    {
        CompoundAction* compoundAction = aznew CompoundAction();

        if (!m_isShowingPalette)
        {
            QPushButton* pushButton = nullptr;
            ScriptCanvasEditor::VariableAutomationRequestBus::BroadcastResult(pushButton, &ScriptCanvasEditor::VariableAutomationRequests::GetCreateVariableButton);

            MouseClickAction* clickAction = aznew MouseClickAction(Qt::MouseButton::LeftButton, pushButton->mapToGlobal(pushButton->rect().center()));

            compoundAction->AddAction(clickAction);
            compoundAction->AddAction(aznew ProcessUserEventsAction());
        }
        else if (!m_isFiltered)
        {
            compoundAction->AddAction(aznew WriteToLineEditAction(m_textFilter, m_variableName.c_str()));
            compoundAction->AddAction(aznew ProcessUserEventsAction());
        }
        else if (!m_indexIsVisible)
        {
            m_graphPalette->scrollTo(m_displayIndex);
            compoundAction->AddAction(aznew ProcessUserEventsAction());
        }

        if (!m_scenePointVisible)
        {
            QRect sceneRect(m_scenePoint, m_scenePoint);
            sceneRect.adjust(-5, -5, 5, 5);

            compoundAction->AddAction(aznew EnsureSceneRectVisibleAction(m_graphId, sceneRect));
            compoundAction->AddAction(aznew ProcessUserEventsAction());
        }

        return compoundAction;
    }
    void CreateVariableNodeFromGraphPalette::SetupAction()
    {
        m_createdNodeId.SetInvalid();
        GraphCanvas::SceneNotificationBus::Handler::BusConnect(m_graphId);

        ClearActionQueue();

        QPoint screenPoint = m_graphPalette->mapToGlobal(m_graphPalette->visualRect(m_displayIndex).center());

        AZ::Vector2 targetPoint;
        GraphCanvas::ViewRequestBus::EventResult(targetPoint, m_viewId, &GraphCanvas::ViewRequests::MapToGlobal, GraphCanvas::ConversionUtils::QPointToVector(m_scenePoint));

        AZ::Vector2 flushTarget = targetPoint + AZ::Vector2(1, 1);

        if (m_modifier == Qt::ShiftModifier)
        {
#if defined(AZ_COMPILER_MSVC)
            AddAction(aznew KeyPressAction(VK_LSHIFT));
#endif
        }
        else if (m_modifier == Qt::AltModifier)
        {
#if defined(AZ_COMPILER_MSVC)
            AddAction(aznew KeyPressAction(VK_LMENU));
#endif
        }

        AddAction(aznew MouseDragAction(screenPoint, GraphCanvas::ConversionUtils::AZToQPoint(targetPoint).toPoint()));        
        AddAction(aznew ProcessUserEventsAction());

        if (m_modifier == Qt::ShiftModifier)
        {
#if defined(AZ_COMPILER_MSVC)
            AddAction(aznew KeyReleaseAction(VK_LSHIFT));
#endif
        }
        else if (m_modifier == Qt::AltModifier)
        {
#if defined(AZ_COMPILER_MSVC)
            AddAction(aznew KeyReleaseAction(VK_LMENU));
#endif
        }

        AddAction(aznew ProcessUserEventsAction());
        AddAction(aznew MouseMoveAction(GraphCanvas::ConversionUtils::AZToQPoint(flushTarget).toPoint()));
        AddAction(aznew ProcessUserEventsAction());

        CompoundAction::SetupAction();
    }

    void CreateVariableNodeFromGraphPalette::OnNodeAdded(const AZ::EntityId& nodeId, bool)
    {
        m_createdNodeId = nodeId;
    }

    void CreateVariableNodeFromGraphPalette::OnActionsComplete()
    {
        GraphCanvas::SceneNotificationBus::Handler::BusDisconnect();
    }

    AZ::EntityId CreateVariableNodeFromGraphPalette::GetCreatedNodeId() const
    {
        return GraphCanvas::GraphUtils::FindOutermostNode(m_createdNodeId);
    }

    ActionReport CreateVariableNodeFromGraphPalette::GenerateReport() const
    {
        if (!m_createdNodeId.IsValid())
        {
            AZStd::string errorString = AZStd::string::format("Failed to create a node for Variable(%s) from the Variable Palette", m_variableName.c_str());
            return AZ::Failure(errorString);
        }

        return CompoundAction::GenerateReport();
    }

    /////////////////////////////
    // ShowGraphVariablesAction
    /////////////////////////////

    void ShowGraphVariablesAction::SetupAction()
    {
        ClearActionQueue();

        bool isShowingGraphPalette = false;
        ScriptCanvasEditor::VariableAutomationRequestBus::BroadcastResult(isShowingGraphPalette, &ScriptCanvasEditor::VariableAutomationRequests::IsShowingGraphVariables);

        if (!isShowingGraphPalette)
        {
            QPushButton* createButton = nullptr;
            ScriptCanvasEditor::VariableAutomationRequestBus::BroadcastResult(createButton, &ScriptCanvasEditor::VariableAutomationRequests::GetCreateVariableButton);

            AddAction(aznew MouseClickAction(Qt::MouseButton::LeftButton, createButton->mapToGlobal(createButton->rect().center())));
            AddAction(aznew ProcessUserEventsAction());
        }

        CompoundAction::SetupAction();
    }

    ActionReport ShowGraphVariablesAction::GenerateReport() const
    {
        bool isShowingGraphPalette = false;
        ScriptCanvasEditor::VariableAutomationRequestBus::BroadcastResult(isShowingGraphPalette, &ScriptCanvasEditor::VariableAutomationRequests::IsShowingGraphVariables);

        if (!isShowingGraphPalette)
        {
            return AZ::Failure<AZStd::string>("Failed to Show Graph Variable");
        }

        return CompoundAction::GenerateReport();
    }
}
